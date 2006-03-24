/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <iostream>

#include "zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/SourceManager.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>

using std::endl;
using namespace zypp;

#include <sqlite3.h>
#include "dbsource/DbAccess.h"

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "query-system"

//-----------------------------------------------------------------------------

static void
sync_sources( sqlite3 *db )
{
    const char *query =
	"CREATE TABLE zsources ("
	"id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
	"zmd_id INTEGER DEFAULT 0, "		// reference to catalogs table for zmd, 0 for zypp
	"alias VARCHAR, "
	"type VARCHAR, "
	"url VARCHAR, "
	"path VARCHAR "
	")";

    int rc = sqlite3_exec( db, query, NULL, NULL, NULL );
    if (rc != SQLITE_OK) {
	ERR << "Can not create 'zsources'[" << rc << "]: " << sqlite3_errmsg( db ) << endl;
	ERR << query << endl;
// ignore error, possibly already exists	return;
    }

    //                                                            1
    query = "SELECT id FROM zsources WHERE zmd_id = 0 AND alias = ?";

    sqlite3_stmt *select_h = NULL;
    rc = sqlite3_prepare( db, query, -1, &select_h, NULL );
    if (rc != SQLITE_OK) {
	ERR << "Can not create select query: " << sqlite3_errmsg( db ) << endl;
	return;
    }

    //                             1      2     3    4
    query = "INSERT INTO zsources (alias, type, url, path) VALUES (?, ?, ?, ?)";

    sqlite3_stmt *insert_h = NULL;
    rc = sqlite3_prepare( db, query, -1, &insert_h, NULL );
    if (rc != SQLITE_OK) {
	ERR << "Can not create insert query: " << sqlite3_errmsg( db ) << endl;
	return;
    }

    SourceManager_Ptr manager = SourceManager::sourceManager();

    try {
	manager->restore( "/" );
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT( excpt_r );
	ERR << "Couldn't restore sources" << endl;
	return;
    }

    std::list<SourceManager::SourceId> sources = manager->allSources();
    MIL << "Found " << sources.size() << " sources" << endl;

    for (std::list<SourceManager::SourceId>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
	Source_Ref source = manager->findSource( *it );

	if (!source) {
	    ERR << "SourceManager can't find source " << *it << endl;
	    continue;
	}


	sqlite3_bind_text( select_h, 1, source.alias().c_str(), -1, SQLITE_STATIC );
	rc = sqlite3_step( select_h );

	bool found = false;
	if (rc == SQLITE_ROW) {
	    found = true;
	    DBG << "Source '" << source.alias() << "' already synched" << endl;
	}
	else if (rc != SQLITE_DONE) {
	    ERR << "rc " << rc << ": " << sqlite3_errmsg( db ) << endl;
	    break;
	}
	sqlite3_reset( select_h );

	if (!found) {
	    DBG << "Syncing source '" << source.alias() << "'" << endl;
	    sqlite3_bind_text( insert_h, 1, source.alias().c_str(), -1, SQLITE_STATIC );
	    std::string type = source.type();
	    if (type.empty()) type = "YaST";
	    sqlite3_bind_text( insert_h, 2, type.c_str(), -1, SQLITE_STATIC );
	    sqlite3_bind_text( insert_h, 3, source.url().asString().c_str(), -1, SQLITE_STATIC );
	    sqlite3_bind_text( insert_h, 4, source.path().asString().c_str(), -1, SQLITE_STATIC );
	    rc = sqlite3_step( insert_h );
	    if (rc != SQLITE_DONE) {
		ERR << "rc " << rc << ": " << sqlite3_errmsg( db ) << endl;
		break;
	    }
	    sqlite3_reset( insert_h );
	}
    }
    return;
}

//-----------------------------------------------------------------------------

int
main (int argc, char **argv)
{
    if (argc != 2) {
	std::cerr << "usage: " << argv[0] << " <database>" << endl;
	return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    MIL << "START query-system " << argv[1] << endl;

    ZYpp::Ptr God = zypp::getZYpp();

    try {
	God->initTarget("/");
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT (excpt_r);
	return 1;
    }

    DbAccess db( argv[1] );
    if (!db.openDb( true ))
	return 1;

    db.writeStore( God->target()->resolvables(), ResStatus::installed, "@system" );

    // sync SourceManager with sources table
    sync_sources( db.db() );

    db.closeDb();

    MIL << "END query-system" << endl;

    return 0;
}
