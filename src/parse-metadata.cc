/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
//
// parse-metadata.cc
//
// parse metadata and pass the info
// - to the database
// - back to zypp, if not there yet (#156139)
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
//
// FIXME rest of sentence missing!

#include <iostream>
#include <cstring>
#include <list>

#include "zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/SourceManager.h>
#include <zypp/SourceFactory.h>
#include <zypp/Source.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>

using namespace std;
using namespace zypp;

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "parse-metadata"

#include <sqlite3.h>
#include <cstring>

#include <sys/stat.h>

#include "dbsource/DbAccess.h"

//----------------------------------------------------------------------------
static SourceManager_Ptr manager;
// manager->store may be expensive
static bool store_needed = false;

static bool
restore_sources ()
{
    try {
	manager->restore("/");
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT (excpt_r);
	ERR << "Couldn't restore sources" << endl;
	return false;
    }
    return true;
}

static bool
store_sources ()
{
    if (store_needed) {
	try {
	    manager->store ("/", true /*metadata_cache*/);
	}
	catch (Exception & excpt_r) {
	    ZYPP_CAUGHT (excpt_r);
	    ERR << "Couldn't store sources" << endl;
	    return false;
	}
    }
    return true;
}

static void
add_source_if_new (const Source_Ref & source, const string & alias)
{
    // only add if it is not there already.
    // findSource throws if the source is not there
    try {
	manager->findSource( alias );
    }
    catch( const Exception & excpt_r ) {
	manager->addSource( source );
	store_needed = true;
    }
}

//----------------------------------------------------------------------------
//Tell yast that we are done storing the updated source. This is
//necessary because when "rug sa" is called by yast, it calls us
//asyncronously

static void
tell_yast()
{
    // yast will delete the file before and afterwards.
    // the name must be synchronized with inst_you.ycp
    // and inst_suse_register.ycp
    const char * flag_file = "/var/lib/zypp/zmd_updated_the_sources";
    int err;
    int fd = creat( flag_file, 0666 );
    if ( fd == -1 ) {
	err = errno;
	ERR << "creating " << flag_file << ": " << strerror( err ) << endl;
    }
    else {
	if ( close( fd ) == -1) {
	    err = errno;
	    ERR << "closing " << flag_file << ": " << strerror( err ) << endl;
	}
    }
}

//----------------------------------------------------------------------------
// upload all zypp sources as catalogs to the database

static void
sync_source( DbAccess & db, Source_Ref source, const string & catalog, const Url & url, bool is_remote = false )
{
    DBG << "sync_source, catalog '" << catalog << "', url '" << url << "', alias '" << source.alias() << "', remote? " << is_remote << endl;

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
	if (!url.getScheme().empty()) {
	    MIL << "Setting source URL  to " << url << endl;
	    source.setUrl( url );
	}

	DBG << "Source provides " << store.size() << " resolvables" << endl;

	db.writeStore( store, ResStatus::uninstalled, catalog.c_str(), is_remote );	// store all resolvables as 'uninstalled'
#if 0
    }
#endif

    return;
}


//----------------------------------------------------------------------------

// metadata types
#define ZYPP "zypp"
#define YUM "yum"

int
main (int argc, char **argv)
{
    atexit (tell_yast);

    if (argc < 5) {
	cerr << "usage: " << argv[0] << " <database> <type> <uri> <catalog id>" << endl;
	return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    //				      database		type		  uri		    catalog
    MIL << "START parse-metadata " << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4] << endl;

    ZYpp::Ptr God = backend::getZYpp( );

    manager = SourceManager::sourceManager();
    if (! restore_sources ())
	return 1;

    DbAccess db( argv[1] );

    if (!db.openDb( true ))		// open for writing
    {
	ERR << "Cannot open database" << endl;
	return 1;
    }

    string type( argv[2] );
    Url uri;
    try {
	uri = Url ( ((argv[3][0] == '/') ? string("file:"):string("")) + argv[3] );
    }
    catch ( const Exception & excpt_r ) {
	ZYPP_CAUGHT( excpt_r );
	cerr << excpt_r.asUserString() << endl;
	return 1;
    }

    string catalog( argv[4] );

    int result = 0;

    if (type == ZYPP)
    {

	MIL << "Doing a zypp->zmd sync" << endl;
	// the source already exists in the catalog, no need to add it

	//
	// find matching source and write its resolvables to the catalog
	//

	SourceManager::Source_const_iterator it;
	for (it = manager->Source_begin(); it !=  manager->Source_end(); ++it) {
	    string src_uri = it->url().asString() + "?alias=" + it->alias();
	    MIL << "Uri " << src_uri << endl;
	    if (src_uri == uri.asString()) {
		sync_source( db, *it, catalog, Url() );
		break;
	    }
	}
	if (it == manager->Source_end()) {
	    WAR << "Source not found" << endl;
	    result = 1;
	}
    }
    else if (strcmp( argv[2], YUM ) == 0)
    {

	MIL << "Doing a zmd->zypp sync" << endl;

	Pathname cache_dir("");
	try {

	    bool is_remote = true;

	    Source_Ref source( SourceFactory().createFrom( uri, Pathname(), catalog, cache_dir ) );
	    
	    // try to create a Url using the catalog (as alias), this is typically
	    // the original location of the source, not the local cache
	    Url url;
	    try {
#warning Need real URL, not catalog
		url = Url( catalog );
		string scheme = url.getScheme();
		if (scheme != "ftp"
		    && scheme != "http"
		    && scheme != "https")
		{
		    is_remote = false;
		}
	    } catch ( const Exception & excpt_r )
	    {
		ZYPP_CAUGHT( excpt_r );
	    }

	    sync_source( db, source, catalog, url, is_remote );

	}
	catch( const Exception & excpt_r )
	{
	    cerr << "3|Cant access repository data at " << argv[3] << endl;
	    ZYPP_CAUGHT( excpt_r );
	    ERR << "Can't access repository at " << argv[3] << endl;
	    result = 1;
	};

    }
    else
    {
	ERR << "Invalid option " << argv[2] << ", expecting '" << ZYPP << "' or '" << YUM << "'" << endl;
	result = 1;
    }

    db.closeDb();

    if (result == 0)
	store_sources ();		// no checking, we're finished anyway

    MIL << "END parse-metadata, result " << result << endl;

    return result;
}
