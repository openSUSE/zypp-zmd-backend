/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <cstdlib>	// setenv
#include <iostream>
#include <string>
#include <list>

#include "zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>
#include <zypp/media/MediaException.h>

#include <zypp/ExternalProgram.h>

using namespace std;
using namespace zypp;

#include <sqlite3.h>
#include "dbsource/DbAccess.h"
#include "dbsource/DbSources.h"

#include "transactions.h"
#include <zypp/solver/detail/ResolverInfo.h>

#include "RpmCallbacks.h"
#include "MediaChangeCallback.h"
#include "MessageResolvableReportCallback.h"

using solver::detail::ResolverInfo_Ptr;

typedef std::list<PoolItem> PoolItemList;

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "transact"

//-----------------------------------------------------------------------------

static void
append_dep_info (ResolverInfo_Ptr info, void *user_data)
{
    string *dep_failure_info = (string *)user_data;
    bool debug = false;

    if (info == NULL) {
	ERR << "append_dep_info(NULL)" << endl;
	return;
    }

    if (getenv ("RCD_DEBUG_DEPS"))
        debug = true;

    if (debug || info->important()) {
	dep_failure_info->append( "\n" );
	if (debug && info->error())
	    dep_failure_info->append( "ERR " );
	if (debug && info->important())
	    dep_failure_info->append( "IMP " );
	dep_failure_info->append( info->message() );
    }
    return;
}


int
main (int argc, char **argv)
{
    if (argc < 2) {
	cerr << "usage: " << argv[0] << " <database> [--test] [--nosignature]" << endl;
	return 1;
    }

    int argp = 2;

    bool dry_run = false;
    bool nosignature = false;
    while (argp < argc) {
	string arg(argv[argp]);
	if (arg == "--test") dry_run = true;
	if (arg == "--nosignature") nosignature = true;
	argp++;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    MIL << "START transact " << argv[1] << (dry_run?" --test":" ") << (nosignature?" --nosignature":"") <<  endl;

    // access the sqlite db

    DbAccess db (argv[1]);
    if (!db.openDb(false))
	return 1;

    // start ZYPP

    ZYpp::Ptr God = backend::getZYpp();
    Target_Ptr target = backend::initTarget( God );

    // load the catalogs and resolvables from sqlite db

    DbSources dbs(db.db());

    const SourcesList & sources = dbs.sources();

    for (SourcesList::const_iterator it = sources.begin(); it != sources.end(); ++it) {
	zypp::ResStore store = it->resolvables();
	MIL << "Catalog " << it->id() << " contributing " << store.size() << " resolvables" << endl;
	God->addResolvables( store, (it->id() == "@system") );
    }

    // now the pool is complete, add transactions

    int count = read_transactions (God->pool(), db.db(), dbs);
    if (count < 0) {
	cerr << "Reading transactions failed." << endl;
	return 1;
    }

    // total number of steps (*2, one for start/preparing, one for install/deleting)
    cout << "0|" << count*2 << endl;

    if (count == 0) {
	MIL << "No transactions found" << endl;
	cout << "4" << endl;
	return 0;
    }

    RpmCallbacks rpm_callbacks;				// init and connect rpm progress callbacks
    MediaChangeCallback med_callback;			// init and connect media change callback
    MessageResolvableReportCallback msg_callback;	// init and connect patch message callback

    int result = 0;

    try {
	PoolItemList x,y,z;

	::setenv( "YAST_IS_RUNNING", "1", 1 );
#warning Must honor nosignature
#if 0
#warning dry_run disabled
	God->target()->commit( God->pool(), 0, x, y, z );
#else
	God->target()->commit( God->pool(), 0, x, y, z, dry_run );
#endif
	ExternalProgram suseconfig( "/sbin/SuSEconfig", ExternalProgram::Discard_Stderr );	// should redirect stderr to logfile
	suseconfig.close();		// discard exit code
    }
    catch ( const media::MediaException & expt_r ) {
	ZYPP_CAUGHT( expt_r );
	result = 1;
	if (med_callback.mediaNr() != 0			// exception due to MediaChange callback ?
	    && !med_callback.description().empty())
	{
	    cerr << "Need media " << med_callback.mediaNr() << ": " << med_callback.description() << endl;
	}
	else {
	    cout << "3|" << expt_r.asUserString() << endl;
	    cerr << expt_r.asString() << endl;
	}
    }
    catch ( const Exception & expt_r ) {
	ZYPP_CAUGHT( expt_r );
	result = 1;
	cout << "3|" << expt_r.asUserString() << endl;
	cerr << expt_r.asString() << endl;
    }

    cout << "4" << endl;

    db.closeDb();

    MIL << "END transact, result " << result << endl;

    return result;
}
