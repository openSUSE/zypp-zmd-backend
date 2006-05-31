/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
//
// update-status helper
//

#include <iostream>
#include <string>

#include "zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>

#include <sqlite3.h>
#include "dbsource/DbAccess.h"
#include "dbsource/DbSources.h"
#include "KeyRingCallbacks.h"

#include <zypp/solver/detail/ResolverInfo.h>

using namespace std;
using namespace zypp;

using solver::detail::ResolverInfo_Ptr;
using solver::detail::ResolverContext_Ptr;

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "update-status"

//-----------------------------------------------------------------------------

typedef struct {
    sqlite3 *db;
    sqlite3_stmt *handle;
} DbHandle;

//
// update 'status' field from item.status()
//
//  handle has to 'bind points'
//    first is status:
//	  0 - undetermined
//	  1 - unneeded
//	  2 - satisfied
//	  3 - broken
//    second is resovable id
//

void
update_db( PoolItem_Ref item, const ResStatus & status, void *data )
{
    DbHandle *dbh = (DbHandle *)data;

    int value = 0;		// default to undetermined
    if (status.isEstablishedUneeded()) value = 1;
    else if (status.isEstablishedSatisfied()) value = 2;
    else if (status.isEstablishedIncomplete()) value = 3;

    sqlite3_bind_int( dbh->handle, 1, value );
    sqlite3_bind_int( dbh->handle, 2, item->zmdid() );

    int rc = sqlite3_step( dbh->handle );
    if (rc != SQLITE_DONE) {
	ERR << "Error updating status: " << sqlite3_errmsg( dbh->db ) << endl;
    }
    sqlite3_reset( dbh->handle );

    return;
}


//
// write 'EstablishField' value back to tables
// FIXME: it only writes patch_details currently
//        but status is computed for _all_ kinds of resolvables
//	  ( just uncomment the second 'const char *sql = ...' statement and remove the first)
//

static bool
write_status( const ResPool & pool, sqlite3 *db, ResolverContext_Ptr context )
{
    MIL << "write_status" << endl;

    sqlite3_stmt *handle = NULL;

    const char *sql = "UPDATE resolvables SET status = ? WHERE id = ?";

    int rc = sqlite3_prepare( db, sql, -1, &handle, NULL );
    if (rc != SQLITE_OK) {
	ERR << "Can not prepare update resolvables clause: " << sqlite3_errmsg (db) << endl;
        return false;
    }

    DbHandle dbh = { db, handle };

    context->foreachMarked( update_db, &dbh );

    sqlite3_finalize( handle );

    return true;
}

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


//-----------------------------------------------------------------------------

int
main (int argc, char **argv)
{
    if (argc != 2) {
	cerr << "usage: " << argv[0] << " <database>" << endl;
	return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    MIL << "START update-status " << argv[1] << endl;

    // access the sqlite db

    DbAccess db (argv[1]);
    if (!db.openDb(false))
	return 1;

    // start ZYPP

    ZYpp::Ptr God = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    // load the catalogs and resolvables from sqlite db

    DbSources dbs(db.db());

    const SourcesList & sources = dbs.sources();

    for (SourcesList::const_iterator it = sources.begin(); it != sources.end(); ++it) {
	zypp::ResStore store = it->resolvables();
	MIL << "Catalog " << it->id() << " contributing " << store.size() << " resolvables" << endl;
	God->addResolvables( store, (it->id() == "@system") );
    }

    bool success = God->resolver()->establishPool();

    MIL << "Solver " << (success?"was":"NOT") << " successful" << endl;

    solver::detail::ResolverContext_Ptr context = God->resolver()->context();
    if (context == NULL) {
	ERR << "No context ?!" << endl;
	return 1;
    }
    if (success) {
	success = write_status( God->pool(), db.db(), context );
    }
    else {
	string dep_failure_info( "Unresolved dependencies:\n" );

	context->foreachInfo( PoolItem_Ref(), -1, append_dep_info, &dep_failure_info );

	cout << dep_failure_info;
	cout.flush();
    }

    db.closeDb();

    MIL << "END update-status, result " << (success ? 0 : 1) << endl;

    return (success ? 0 : 1);
}
