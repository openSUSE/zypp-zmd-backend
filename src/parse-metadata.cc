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
#include <zypp/base/Sysconfig.h>

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

#define SWMAN_PATH "/etc/sysconfig/sw_management"
#define SWMAN_ZMD2ZYPP_TAG "SYNC_ZMD_TO_ZYPP"

//----------------------------------------------------------------------------
static SourceManager_Ptr manager;
// manager->store may be expensive

// query system for installed packages
static int
query_system ( ZYpp::Ptr zypp, const Pathname &rpm_prefix, const std::string &dbfile )
{
  MIL << "-------------------------------------" << endl;
  MIL << "START parse-metadata @system " << dbfile << endl;

  Target_Ptr target = backend::initTarget( zypp, rpm_prefix );

  DbAccess db( dbfile );
  if (!db.openDb( true )) {
    return 1;
  }

  db.emptyCatalog("@system");
  db.writeStore( zypp->target()->resolvables(), ResStatus::installed, "@system", ZYPP_OWNED );
  db.closeDb();

  MIL << "END parse-metadata @system, result 0" << endl;

  return 0;
}

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
  try
  {
    ResStore store = source.resolvables();
    if (!url.getScheme().empty())
    {
      MIL << "Setting source URL  to " << url << endl;
      source.setUrl( url );
    }

    DBG << "Source provides " << store.size() << " resolvables" << endl;

    // clean up db if we fail here
    result = 1;
    
    // FIXME add the smart algorithm here
    db.emptyCatalog(catalog);
    db.writeStore( store, ResStatus::uninstalled, catalog.c_str(), owner );	// store all resolvables as 'uninstalled'
    db.updateCatalogChecksum( catalog, source.checksum(), source.timestamp() );
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

static int
parse_metadata( Ownership owner, const std::string &p_dbfile, const std::string &p_path, const std::string &p_url, const std::string &p_catalog )
{
  DbAccess db( p_dbfile );		// the zmd.db

  if (!db.openDb( true ))		// open for writing
  {
    cerr << "1|Cannot open database " << p_dbfile << endl;
    ERR << "Cannot open database" << endl;
    return 1;
  }

    // check for "...;alias=..." and replace with "...&alias=..." (#168030)
  string checkpath ( p_url );
  string::size_type aliaspos = checkpath.find( ";alias=" );
  if (aliaspos != string::npos) {
    checkpath.replace( aliaspos, 1, "&" );
  }

  string checkurl( p_path );
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
  string catalog( p_catalog );

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

  if (source)
  {
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

    try
    {
      if (owner == ZMD_OWNED) {		// use zmd downloaded metadata:
        source = SourceFactory().createFrom( pathurl, Pathname(), catalog, Pathname() );
        // zmd provided the data locally, 'source' has a local path, pass the real uri to sync_source
        result = sync_source( db, source, catalog, uri, owner );
      }
      else
      { // let zypp do the download
        source = SourceFactory().createFrom( uri, Pathname(), catalog, Pathname() );
        // new zypp source, 'source' already has the real url, pass empty uri to sync_source
        result = sync_source( db, source, catalog, Url(), owner );
      }
    }
    catch( const Exception & excpt_r )
    {
      cerr << "1|Can't add repository at " << uri << ": " << joinlines( excpt_r.asUserString() ) << endl;
      ZYPP_CAUGHT( excpt_r );
      ERR << "Can't add repository at " << uri << endl;
      result = 1;
    }

    if (result != 0)
      goto finish;

    // See bug #168739

    bool sync_all_sources = false;
    map<string,string> data = zypp::base::sysconfig::read( SWMAN_PATH );
    map<string,string>::const_iterator it = data.find( SWMAN_ZMD2ZYPP_TAG );
    if (it != data.end())
      sync_all_sources = (it->second == "always");

    if (!sync_all_sources && owner == ZMD_OWNED)
    {
      MIL << "*not* adding to zypp - SYNC_ZMD_TO_ZYPP in"
       " /etc/sysconfig/sw_management is not set to 'always' "
       " and owner is ZMD" << endl;
      goto finish;
    }

    struct stat st;
    if (stat( "/var/lib/zypp/sources-being-processed-by-yast", &st ) == 0) {
      MIL << "Processed by YaST, exiting" << endl;
      goto finish;
    }

    MIL << "Source not found, adding" << endl;

    try
    {
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


//----------------------------------------------------------------------------

// metadata owners
#define SYSTEM "system"
#define ZYPP "zypp"
#define ZMD "yum"	// zmd claims "yum" for itself

int
main (int argc, char **argv)
{
    if (argc < 6)
    {
      cerr << "1|usage: " << argv[0] << " <database> <owner> <uri> <path> <catalog id>" << endl;
      return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
      zypp::base::LogControl::instance().logfile( logfile );
    else
      zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    //                                     database    owner               uri                path            catalog
    MIL << "START parse-metadata " << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4] << " " << argv[5] << endl;

    string owned_by( argv[2] );		// "zypp" or "yum"
    Ownership owner = ZYPP_OWNED;

    if (owned_by == ZYPP)
    {
      MIL << "Zypp owned" << endl;
    }
    else if (owned_by == SYSTEM)
    {
      MIL << "owned by system" << endl;
    }
    else if (owned_by == ZMD)
    {
      MIL << "ZMD owned" << endl;
      owner = ZMD_OWNED;
    }
    else
    {
      cerr << "1|Invalid option " << argv[2] << ", expecting '" << ZYPP << "' or '" << ZMD << "'" << endl;
      ERR << "Invalid option " << argv[2] << ", expecting '" << ZYPP << "' or '" << ZMD << "'" << endl;
      return 1;
    }

    ZYpp::Ptr God = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    if ( (owned_by == ZYPP) || (owned_by == ZMD) )
    {
      backend::initTarget( God );
      return parse_metadata( owner, argv[1] /* zmd db */, argv[3] /* url */, argv[4] /* path */, argv[5] /*catalog */);
    }
    else if ( ( owned_by == SYSTEM ) && ( string(argv[5]) == "@system" ) ) 
    {
      backend::initTarget( God, argv[3] );
      return query_system ( God, argv[3] /* url, used as rpm prefix */, argv[1] /* zmd db */ );
    }
    else if ( ( owned_by == SYSTEM ) && ( string(argv[5]) != "@system" ) ) 
    {
      cerr << "1|Invalid option " << argv[5] << ", expecting '@system'" << endl;
      ERR << "Invalid option " << argv[5] << ", expecting '@system'" << endl;
    }
    return 1;
}
