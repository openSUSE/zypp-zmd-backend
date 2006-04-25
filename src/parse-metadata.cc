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

// metadata owners
#define ZYPP "zypp"
#define ZMD "yum"	// zmd claims "yum" for itself

int
main (int argc, char **argv)
{
    if (argc < 5) {
	cerr << "3|usage: " << argv[0] << " <database> <type> <uri> <catalog id>" << endl;
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

    string owner( argv[2] );		// "zypp" or "yum"

    if (owner == ZYPP)
    {
	MIL << "Zypp owned" << endl;
    }
    else if (owner == ZMD) {
	MIL << "ZMD owned" << endl;
    }
    else {
	ERR << "Invalid option " << argv[2] << ", expecting '" << ZYPP << "' or '" << ZMD << "'" << endl;
	return 1;
    }

    ZYpp::Ptr God = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    backend::initTarget( God );

    manager = SourceManager::sourceManager();
    if (! restore_sources ()) {
	return 1;
    }

    DbAccess db( argv[1] );		// the zmd.db

    if (!db.openDb( true ))		// open for writing
    {
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
	cerr << "3|" << excpt_r.asUserString() << endl;
	return 1;
    }

    // If the URL has an alias, it must be ZYPP owned.

    if (owner == ZMD
	&& !urialias.empty())
    {
	ERR << "Bad paremeters, yum-type url with alias" << endl;
	cerr << "3|Bad paremeters, yum-type url with alias" << endl;
	return 1;
    }

    // the catalog id to use when writing to zmd.db
    string catalog( argv[5] );

    int result = 0;

    MIL << "Uri '" << uri << "', Alias '" << urialias << "', Path '" << pathurl << "'" << endl;

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

    SourceManager::Source_const_iterator it;
    for (it = manager->Source_begin(); it !=  manager->Source_end(); ++it) {

	if (urialias.empty()) {					// urialias empty -> not coming from yast

	    if (uri.asString() == it->url().asString()) {	// url already known ?

		if (owner == ZMD) {				//  and owned by ZMD ?
		    MIL << "Found url, source already known to zypp" << endl;
		    sync_source( db, *it, catalog, Url(), owner == ZMD );
		    break;
		}
		else {						// --type=zypp used but no alias given
		    ERR << "Duplicate source, url already known" << endl;
		    cerr << "3|Duplicate source, url already known" << endl;
		    break;
		}
	    }
	}
	else if (urialias == it->alias()) {			// urialias matches zypp one
	    MIL << "Found alias, source already known to zypp" << endl;
	    sync_source( db, *it, catalog, Url(), owner == ZMD );
	    break;
	}
    }

    // if the source is not known in zypp, add it

    if (it == manager->Source_end()) {

	if (!urialias.empty()) {				// alias given but not found in zypp -> error
	    cerr << "3|Unknown alias '" << urialias << "' passed." << endl;
	    ERR << "Unknown alias '" << urialias << "' passed." << endl;
	    goto finish;
	}

	MIL << "Source not found, creating" << endl;

	Source_Ref source;
	try {
	    if (owner == ZMD) {		// use zmd downloaded metadata:
		source = SourceFactory().createFrom( pathurl, Pathname(), catalog, Pathname() );
		sync_source( db, source, catalog, uri, owner == ZMD );
	    }
	    else {			// let zypp do the download
		source = SourceFactory().createFrom( uri, Pathname(), catalog, Pathname() );
		sync_source( db, source, catalog, Url(), owner == ZMD );
	    }
	}
	catch( const Exception & excpt_r ) {
	    cerr << "3|Can't add repository at " << uri << endl;
	    ZYPP_CAUGHT( excpt_r );
	    ERR << "Can't add repository at " << uri << endl;
	    goto finish;
	}

// See bug #168739 

	if (owner == ZMD) {
	    MIL << "Owner is ZMD, *not* adding to zypp" << endl;
	    goto finish;
	}

	MIL << "Source not found, adding" << endl;

	try {
	    manager->addSource( source );
	    manager->store( "/", true /*metadata_cache*/ );
	}
	catch (Exception & excpt_r) {
	    cerr << "3|Can't store source to zypp" << endl;
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
