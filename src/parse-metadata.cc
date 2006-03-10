/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
//
// parse-metadata.cc
//
// parse-metadata <zmd.db> <metadata type> <path> <catalog id>
//
// metadata type can be currently either 'yum' or 'installation'.
// path would be the path on the local file system. Here's an example for
// yum:
// 
// parse-metadata zmd.db yum /var/cache/zmd/foo/ 2332523
// 
// The helper is guaranteed to find /var/cache/zmd/foo/repodata/repomd.xml
// and all the other files referenced from the repomd.xml. For example, if
// the repomd.xml has

#include <iostream>
#include <cstring>
#include <list>

#include "zmd-backend.h"

#include "zypp/ZYpp.h"
#include "zypp/ZYppFactory.h"
#include "zypp/SourceManager.h"
#include "zypp/SourceFactory.h"
#include "zypp/Source.h"
#include "zypp/base/Logger.h"
#include "zypp/base/Exception.h"

using namespace std;
using namespace zypp;

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "parse-metadata"

#include <sqlite3.h>
#include <cstring>

#include <sys/stat.h>

#include "dbsource/DbAccess.h"

//----------------------------------------------------------------------------
// upload all zypp sources as catalogs to the database

static void
sync_source( DbAccess & db, Source_Ref source, string catalog )
{
    DBG << "sync_source, catalog '" << catalog << "', alias '" << source.alias() << "'" << endl;

#if 0	// ZMD does this
    if (db.haveCatalog( catalog ) ) {
	db.removeCatalog( catalog );		// clean old entries first
    }
#endif

#if 0 // catalogs table is locked
    DBG << "Updating catalog '" << catalog << "', setting description to '" << source.url().asString() << "'" << endl;
    // save the URL in the description attribute
    db.updateCatalog( catalog, "", "", source.url().asString() );
#endif

#if 0	// ZMD does this
    string name = source.zmdName();
    if (name.empty()) name = source.url().asString();
    string desc = source.zmdDescription();
    if (desc.empty()) desc = source.vendor();

    if (db.insertCatalog( catalog, name, catalog, desc )) {		// create catalog
#endif

	ResStore store = source.resolvables();

	DBG << "Source provides " << store.size() << " resolvables" << endl;

	db.writeStore( store, ResStatus::uninstalled, catalog.c_str() );	// store all resolvables as 'uninstalled'
#if 0
    }
#endif

    return;
}


//
// find matching source and write its resolvables to the catalog
//

static void
sync_catalog( DbAccess & db, const string & path, const string & catalog )
{
    MIL << "sync_catalog(..., " << path << ", " << catalog << ")" << endl;

    SourceManager_Ptr manager = SourceManager::sourceManager();

    try {
	manager->restore("/");
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT (excpt_r);
	ERR << "Couldn't restore sources" << endl;
	return;
    }

    list<SourceManager::SourceId> sources = manager->allSources();
    MIL << "Found " << sources.size() << " sources" << endl;

    for (list<SourceManager::SourceId>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
	Source_Ref source = manager->findSource( *it );
	if (!source) {
	    ERR << "SourceManager can't find source " << *it << endl;
	    continue;
	}
	if (!source.enabled())
	    continue;

	if ((path == "/installation") 		// just sync the first source
	    || (catalog == source.alias()))	// or the matching one
	{ 
	    sync_source( db, source, catalog );
	    break;
	}
    }
    return;
}

//----------------------------------------------------------------------------

// metadata types
#define ZYPP "zypp"
#define YUM "yum"

int
main (int argc, char **argv)
{
    if (argc != 5) {
	cerr << "usage: " << argv[0] << " <database> <type> <path> <catalog id>" << endl;
	return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    //				      database		type		  path/uri	    catalog/alias
    MIL << "START parse-metadata " << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4] << endl;

    ZYpp::Ptr God = zypp::getZYpp();

    DbAccess db(argv[1]);

    db.openDb( true );		// open for writing

    if (strcmp( argv[2], ZYPP) == 0) {
	MIL << "Doing a catalog sync" << endl;
	sync_catalog( db, argv[3], argv[4] );
    }
    else if (strcmp( argv[2], YUM) == 0) {
	MIL << "Doing a cached source sync" << endl;
	Pathname p;
	Url url( string("file:") + argv[3] );
	string alias( argv[4] );
	Locale lang( "en" );

	Pathname cache_dir("");
	Source_Ref source( SourceFactory().createFrom(url, p, alias, cache_dir) );

	sync_source ( db, source, alias );
    }
    else {
	ERR << "Invalid option " << argv[2] << ", expecting '" << ZYPP << "' or '" << YUM << "'" << endl;
    }

    db.closeDb();

    MIL << "END parse-metadata" << endl;

    return 0;
}
