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
#include "KeyRingCallbacks.h"

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
sync_source( DbAccess & db, Source_Ref source, const string & catalog, const Url & url, bool zmd_owned )
{
    DBG << "sync_source, catalog '" << catalog << "', url '" << url << "', alias '" << source.alias() << ", zmd_owned " << zmd_owned << endl;

    ResStore store = source.resolvables();
    if (!url.getScheme().empty()) {
	MIL << "Setting source URL  to " << url << endl;
	source.setUrl( url );
    }

    DBG << "Source provides " << store.size() << " resolvables" << endl;

    db.writeStore( store, ResStatus::uninstalled, catalog.c_str(), zmd_owned );	// store all resolvables as 'uninstalled'

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
    //				      database		type		  uri		    path              catalog
    MIL << "START parse-metadata " << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4] << " " << argv[5] << endl;

    ZYpp::Ptr God = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    backend::initTarget( God );

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
    Pathname path( argv[3] );

    Url uri;
    Url pathurl;
    try {
	uri = Url ( ((argv[3][0] == '/') ? string("file:"):string("")) + argv[3] );
	pathurl = Url ( ((argv[4][0] == '/') ? string("file:"):string("")) + argv[4] );
    }
    catch ( const Exception & excpt_r ) {
	ZYPP_CAUGHT( excpt_r );
	cerr << excpt_r.asUserString() << endl;
	return 1;
    }

    string catalog( argv[5] );

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
	    string src_uri = it->url().asString();
	    MIL << "Uri " << src_uri << endl;
	    if (src_uri == uri.asString()) {				// check url first
		sync_source( db, *it, catalog, Url(), false );
		break;
	    }
	    src_uri += "?alias=" + it->alias();
	    MIL << "Uri " << src_uri << endl;
	    if (src_uri == uri.asString()) {				// then check <url>?alias=<alias>
		sync_source( db, *it, catalog, Url(), false );
		break;
	    }
	}
	if (it == manager->Source_end()) {
	    MIL << "Source not found, adding" << endl;

	    Source_Ref source;
	    try {
		source = SourceFactory().createFrom( uri, Pathname(), catalog, Pathname() );
		sync_source( db, source, catalog, Url(), false );
	    }
	    catch( const Exception & excpt_r ) {
		cerr << "3|Can't add repository at " << uri << endl;
		ZYPP_CAUGHT( excpt_r );
		ERR << "Can't add repository at " << uri << endl;
		result = 1;
	    }

	    if (result == 0) {
		try {
		    manager->addSource( source );
		    manager->store( "/", true /*metadata_cache*/ );
		}
		catch (Exception & excpt_r) {
		    cerr << "3|Can't store zypp repository" << endl;
		    ZYPP_CAUGHT (excpt_r);
		    ERR << "Couldn't store sources" << endl;
		    result = 1;
		}
	    }
	}
    }
    else if (type == YUM)
    {

	MIL << "Doing a zmd->zypp sync" << endl;

	try {

	    Source_Ref source( SourceFactory().createFrom( pathurl, Pathname(), catalog, Pathname() ) );
	    
	    sync_source( db, source, catalog, uri, true );		// yum is always zmd_owned

	}
	catch( const Exception & excpt_r )
	{
	    cerr << "3|Cant access repository data at " << path << endl;
	    ZYPP_CAUGHT( excpt_r );
	    ERR << "Can't access repository at " << path << endl;
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
