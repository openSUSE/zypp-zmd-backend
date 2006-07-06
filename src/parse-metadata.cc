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
//  <location href="repodata/primary.xml"/>
// then the file is locally at /var/cache/zmd/foo/repodata/primary.xml.
//
// The helper would parse all metadata, and write all resolvables to the
// database, using the catalog id as a catalog for the resolvables. 
// At refresh, zmd would remove the catalog from the database, triggering
// the removal of all resolvables which which have the same catalog id,
// re-download all metadata (if needed) and call the helper the same way.
// package_details.package_url will be added and it should be set by the
// helper without the base url so that the daemon could construct the full
// path from url + "/" + package_details.package_url.
// After downloading the package, zmd fills in package_details.package_filename
// with the full path on the local file system.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <list>

#include "dbsource/zmd-backend.h"

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

#include "dbsource/utils.h"
#include "dbsource/DbAccess.h"
#include "KeyRingCallbacks.h"

//----------------------------------------------------------------------------
static SourceManager_Ptr manager;
// manager->store may be expensive

//----------------------------------------------------------------------------
// upload all zypp sources as catalogs to the database

// If url is non-empty, it defines the real url where the metadata comes from
//  Usually zmd downloads metadata and provides is via a local path
//  However, this is not useful for zypp since other applications linking against
//  zypp (e.g. YaST) need the real url in order to work with it.
// So in the special case of local metadata (since already downloaded by zmd), the
// real url is passed here and set in the source. This way, storing the source in
// zypp makes all information available to all zypp applications.
//
static int
sync_source( DbAccess & db, Source_Ref source, const string & catalog, const Url & url, Ownership owner )
{
    int result = 0;

    DBG << "sync_source, catalog '" << catalog << "', url '" << url << "', alias '" << source.alias() << ", owner " << owner << endl;

    try {
	ResStore store = source.resolvables();
	if (!url.getScheme().empty()) {
	    MIL << "Setting source URL  to " << url << endl;
	    source.setUrl( url );
	}

	DBG << "Source provides " << store.size() << " resolvables" << endl;

	// clean up db if we fail here
	result = 1;
	db.writeStore( store, ResStatus::uninstalled, catalog.c_str(), owner );	// store all resolvables as 'uninstalled'
	result = 0;
    }
    catch ( const Exception & excpt_r ) {
	ZYPP_CAUGHT( excpt_r );
	cerr << "1|Can't parse repository data: " << joinlines( excpt_r.asUserString() ) << endl;
	// don't (!) set result=1 here. If we fail in source.resolvables() above, we simply leave
	// the db untouched.
    }

    if (result != 0) {	// failed in db.writeStore(), see #189308
	ERR << "Write to database failed, cleaning up" << endl;
    	db.emptyCatalog( catalog.c_str() );
    }
    return result;
}


//----------------------------------------------------------------------------

// metadata owners
#define ZYPP "zypp"
#define ZMD "yum"	// zmd claims "yum" for itself

int
main (int argc, char **argv)
{
    if (argc < 6) {
	cerr << "1|usage: " << argv[0] << " <database> <owner> <uri> <path> <catalog id>" << endl;
	return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    //				      database		owner		  uri		    path              catalog
    MIL << "START parse-metadata " << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4] << " " << argv[5] << endl;

    string owned_by( argv[2] );		// "zypp" or "yum"
    Ownership owner = ZYPP_OWNED;

    if (owned_by == ZYPP)
    {
	MIL << "Zypp owned" << endl;
    }
    else if (owned_by == ZMD) {
	MIL << "ZMD owned" << endl;
	owner = ZMD_OWNED;
    }
    else {
	cerr << "1|Invalid option " << argv[2] << ", expecting '" << ZYPP << "' or '" << ZMD << "'" << endl;
	ERR << "Invalid option " << argv[2] << ", expecting '" << ZYPP << "' or '" << ZMD << "'" << endl;
	return 1;
    }

    ZYpp::Ptr God = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    backend::initTarget( God );

    DbAccess db( argv[1] );		// the zmd.db

    if (!db.openDb( true ))		// open for writing
    {
	cerr << "1|Cannot open database " << argv[1] << endl;
	ERR << "Cannot open database" << endl;
	return 1;
    }

    // check for "...;alias=..." and replace with "...&alias=..." (#168030)
    string checkpath ( argv[3] );
    string::size_type aliaspos = checkpath.find( ";alias=" );
    if (aliaspos != string::npos) {
	checkpath.replace( aliaspos, 1, "&" );
    }

    string checkurl( argv[4] );
    aliaspos = checkurl.find( ";alias=" );
    if (aliaspos != string::npos) {
	checkurl.replace( aliaspos, 1, "&" );
    }


    Url uri;
    Url pathurl;
    url::ParamMap uriparams;
    string urialias;

    // Url() constructor might throw
    try {
	uri = Url ( ((checkpath[0] == '/') ? string("file:"):string("")) + checkpath );
	uriparams = uri.getQueryStringMap();	// extract parameters
	url::ParamMap::const_iterator it = uriparams.find( "alias" );
	if (it != uriparams.end()) {
	    urialias = it->second;
	}
	pathurl = Url ( ((checkurl[0] == '/') ? string("file:"):string("")) + checkurl );
    }
    catch ( const Exception & excpt_r ) {
	ZYPP_CAUGHT( excpt_r );
	cerr << "1|" << joinlines( excpt_r.asUserString() ) << endl;
	return 1;
    }

    // If the URL has an alias, it must be ZYPP owned.

    if (owner == ZMD_OWNED
	&& !urialias.empty())
    {
	ERR << "Bad paremeters, yum-type url with alias" << endl;
	cerr << "1|Bad paremeters, yum-type url with alias" << endl;
	return 1;
    }

    // the catalog id to use when writing to zmd.db
    string catalog( argv[5] );

    int result = 0;

    MIL << "Uri '" << uri << "', Alias '" << urialias << "', Path '" << pathurl << "'" << endl;

    if (owner == ZYPP_OWNED) {
	backend::addZyppOwned( catalog );
    }

    manager = SourceManager::sourceManager();

    //
    // find matching source and write its resolvables to the catalog
    //
    // consider two cases
    // 1. source added via YaST
    // 2. source added via ZMD (rug, zen-installer, ...)
    //
    // case 1: YaST calls "rug sa <url>" but adds an alias to the URL
    //	This alias parameter is compared to the alias values of the source
    //	If the correct source cannot be found, we report an error
    //	Else we get the source resolvables and write it to zmd.db (-> sync_source)
    //
    // case 2: A source might be owned by zypp or zmd (-> 'owner' parameter, see above)
    //	For zmd owned sources, we first check if the url is already known. This is
    //   safe since the url is unique within zmd.
    //
    // If the source is not known within zypp, we add it to zypp so it can be
    // used by applications linking against zypp.
    //

    Source_Ref source = backend::findSource( manager, urialias, uri );

    if (source) {
// Done in sources factoryInit
// 	if (source.autorefresh()) {
// 	    try {
// 		source.refresh();				// refresh zypp-known autorefresh source
// 	    }
// 	    catch( const Exception & excpt_r ) {
// 		cerr << "3|Can't refresh " << uri << ": " << joinlines( excpt_r.asUserString() ) << endl;
// 		ZYPP_CAUGHT( excpt_r );
// 		ERR << "Can't refresh " << uri << endl;
// 		// continue with current data (unrefreshed)
// 	    }
// 	}
	// since its known by url, it already has a real Url, no need to pass one
	result = sync_source( db, source, catalog, Url(), owner );
    }
    else {    // if the source is not known in zypp, add it

	if (!urialias.empty()) {				// alias given but not found in zypp -> error
	    cerr << "1|Unknown alias '" << urialias << "' passed." << endl;
	    ERR << "Unknown alias '" << urialias << "' passed." << endl;
	    goto finish;
	}

	MIL << "Source not found, creating" << endl;

	try {
	    if (owner == ZMD_OWNED) {		// use zmd downloaded metadata:
		source = SourceFactory().createFrom( pathurl, Pathname(), catalog, Pathname() );
		// zmd provided the data locally, 'source' has a local path, pass the real uri to sync_source
		result = sync_source( db, source, catalog, uri, owner );
	    }
	    else {			// let zypp do the download
		source = SourceFactory().createFrom( uri, Pathname(), catalog, Pathname() );
		// new zypp source, 'source' already has the real url, pass empty uri to sync_source
		result = sync_source( db, source, catalog, Url(), owner );
	    }
	}
	catch( const Exception & excpt_r ) {
	    cerr << "1|Can't add repository at " << uri << ": " << joinlines( excpt_r.asUserString() ) << endl;
	    ZYPP_CAUGHT( excpt_r );
	    ERR << "Can't add repository at " << uri << endl;
	    result = 1;
	}

	if (result != 0)
	    goto finish;

// See bug #168739 

	if (owner == ZMD_OWNED) {
	    MIL << "Owner is ZMD, *not* adding to zypp" << endl;
	    goto finish;
	}

	struct stat st;
	if (stat( "/var/lib/zypp/sources-being-processed-by-yast", &st ) == 0) {
	    MIL << "Processed by YaST, exiting" << endl;
	    goto finish;
	}

	MIL << "Source not found, adding" << endl;

	try {
	    manager->addSource( source );
	    manager->store( "/", true /*metadata_cache*/ );
	}
	catch (Exception & excpt_r) {
	    cerr << "1|Can't store source to zypp" << endl;
	    ZYPP_CAUGHT (excpt_r);
	    ERR << "Couldn't store source to zypp" << endl;
	    result = 1;
	}

    }
finish:

    db.closeDb();

    MIL << "END parse-metadata, result " << result << endl;

    return result;
}
