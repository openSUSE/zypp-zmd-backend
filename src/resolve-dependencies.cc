/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <iostream>
#include <string>

#include "zmd-backend.h"

#include "zypp/ZYpp.h"
#include "zypp/ZYppFactory.h"
#include "zypp/base/Logger.h"
#include "zypp/base/Exception.h"

using namespace std;
using namespace zypp;

#include <sqlite3.h>
#include "dbsource/DbAccess.h"
#include "dbsource/DbSources.h"

#include "transactions.h"
#include "zypp/solver/detail/ResolverInfo.h"

using solver::detail::ResolverInfo_Ptr;

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "resolve-dependencies"


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
    DBG << "append_dep_info(" << *info << ")" << endl;
    DBG << "append_dep_info([" << info->message() << "])" << endl;

    if (getenv ("RCD_DEBUG_DEPS"))
        debug = true;

    if (debug || info->important()) {
	string s = *dep_failure_info;
	s += "\n";
	if (debug && info->error())
	    s += "ERR ";
	if (debug && info->important())
	    s += "IMP ";
	s += info->message();
	*dep_failure_info = s;
    }
    return;
}


int
main (int argc, char **argv)
{
    if (argc != 2) {
	cerr << "usage: " << argv[0] << " <database> [verify]" << endl;
	return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    MIL << "START resolve-dependencies " << argv[1] << endl;

    // access the sqlite db

    DbAccess db (argv[1]);
    if (!db.openDb(false))
	return 1;

    // start ZYPP

    ZYpp::Ptr God = zypp::getZYpp();

    // load the catalogs and resolvables from sqlite db

    DbSources dbs(db.db());

    const SourcesList & sources = dbs.sources();

    for (SourcesList::const_iterator it = sources.begin(); it != sources.end(); ++it) {
	zypp::ResStore store = it->resolvables();
	MIL << "Catalog " << it->id() << " contributing " << store.size() << " resolvables" << endl;
	God->addResolvables( store, (it->id() == "@system") );
    }

    // now the pool is complete, add transactions

    if (!read_transactions (God->pool(), db.db(), dbs))
	return 1;

    bool success;
    if (argc == 3) {
	success = God->resolver()->verifySystem();
    }
    else {
	success = God->resolver()->resolvePool();
    }
    MIL << "Solver " << (success?"was":"NOT") << " successful" << endl;

    solver::detail::ResolverContext_Ptr context = God->resolver()->context();
    if (context == NULL) {
	ERR << "No context ?!" << endl;
	return 1;
    }
    if (success) {
	success = write_transactions( God->pool(), db.db(), context );
    }
    else {
	string dep_failure_info( "Unresolved dependencies:\n" );

	context->foreachInfo( PoolItem_Ref(), -1, append_dep_info, &dep_failure_info );

	MIL << "failure info: " << dep_failure_info << endl;
	cout << dep_failure_info;
	cout.flush();
    }

    db.closeDb();

    MIL << "END resolve-dependencies" << endl;

    return (success ? 0 : 1);
}
