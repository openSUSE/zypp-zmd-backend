/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
// zmd-backend.cc
// ZMD backend helpers

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <list>
#include <fstream>

#include "utils.h"
#include "zmd-backend.h"

#include <zypp/zypp_detail/ZYppReadOnlyHack.h>

using namespace zypp;
using namespace std;

namespace backend {

ZYpp::Ptr
getZYpp( bool readonly )
{
    if (readonly)
	zypp::zypp_readonly_hack::IWantIt();

    ZYpp::Ptr Z = NULL;
    try {
	Z = zypp::getZYpp();
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT (excpt_r);
	cerr << "1|" << excpt_r.asUserString() << endl;
	cout << excpt_r.asUserString() << endl;
	exit(1);
    }
    return Z;
}

Target_Ptr
initTarget( ZYpp::Ptr Z, const Pathname &root )
{
  Target_Ptr T;

  try
  {
    Z->initializeTarget(root);	// its always "/", and we never populate the pool
    T = Z->target();
  }
  catch (Exception & excpt_r)
  {
    ZYPP_CAUGHT (excpt_r);
    cerr << "3|" << joinlines( excpt_r.asUserString() ) << endl;
    cout << excpt_r.asString() << endl;
    exit(1);
  }
  return T;
}


// restore source by Alias or by Url
// prefer by Alias, use Url if Alias is empty
// will restore all sources if Alias and Url are empty

bool
restoreSources( SourceManager_Ptr manager, const string & alias, const string & url )
{
    try {
	manager->restore( "/", true, alias, url );
    }
    catch (const Exception & excpt_r) {
	cerr << "2|Can't restore sources: " << joinlines( excpt_r.asUserString() ) << endl;
	ZYPP_CAUGHT (excpt_r);
	ERR << "Couldn't restore sources, alias '" << alias << "', url '" << url << "'" << endl;
	return false;
    }
    return true;
}


// find (and restore) source by Alias or by Url
// prefer by Alias, use Url if Alias is empty

Source_Ref
findSource( SourceManager_Ptr manager, const string & alias, const Url & url )
{
    Source_Ref source;

    // restore matching sources

    if (backend::restoreSources( manager, alias, alias.empty() ? url.asString() : "" )) {
	try {
	    if (alias.empty()) {
		source = manager->findSourceByUrl( url );
	    }
	    else {
		source = manager->findSource( alias );
	    }
	}
	catch (const Exception & excpt_r) {
	    // dont log to ZMD, failing is actually ok since e.g. parse-metadata might just
	    // create this source. There is currently no way to tell if the call to parse-metadata
	    // is 'create' or 'refresh'.
//	    cerr << "1|Can't find source: " << joinlines( excpt_r.asUserString() ) << ", alias '" << alias << "', url '" << url << endl;
	    ZYPP_CAUGHT (excpt_r);
	    WAR << "Can't find source" << ", alias '" << alias << "', url '" << url << endl;
	}
    }

    return source;
}


#define ZYPP_CATALOGS "/var/lib/zmd/zypp-owned-catalogs"

// make filename settable, used e.g. by testsuite
const string &
zyppOwnedFilename( const string & name )
{
    static string filename( ZYPP_CATALOGS );
    if ( !name.empty() )
	filename = name;

    return filename;
}


// get list of current 'zypp' owned catalogs (output empty)
// or write list (output non-empty)

StringList
zyppOwnedCatalogs( StringList *output = NULL )
{
    static StringList catalogs;
    fstream file;

    string filename( zyppOwnedFilename() );

    if ( output == NULL ) {		// read catalogs
	if ( !catalogs.empty() )
	    return catalogs;		// return local copy if read before
	struct stat st;
	if ( stat( filename.c_str(), &st ) != 0 ) {
	    MIL << filename << " not existing" << endl;
	}
	file.open( filename.c_str(), ios::in );
	while ( file ) {
	    string catalog;
	    if (std::getline( file, catalog, '\n' ).eof())
		break;
	    catalogs.push_back( catalog );
	}
	file.close();
    }
    else {				// output set, write catalogs
	catalogs = *output;
	file.open( filename.c_str(), ios::out | ios::trunc );
	if ( file ) {
	    for (StringList::const_iterator it = catalogs.begin(); it != catalogs.end(); ++it) {
		file << *it << endl;
	    }
	    file.close();
	}
	else {
	    ERR << "Can not open " << filename << " for writing." << endl;
	}
    }
    return catalogs;
}


// check if given catalog is zypp owned (-> transact)

bool
isZyppOwned( std::string catalog )
{
    StringList catalogs( zyppOwnedCatalogs( ) );
    for (StringList::const_iterator it = catalogs.begin(); it != catalogs.end(); ++it) {
	if ( *it == catalog ) return true;
    }
    return false;
}


// add catalog as zypp owned (-> parse-metadata)

void
addZyppOwned( std::string catalog )
{
    StringList catalogs( zyppOwnedCatalogs( ) );
    for (StringList::const_iterator it = catalogs.begin(); it != catalogs.end(); ++it) {
	if ( *it == catalog ) return;
    }
    catalogs.push_back( catalog );
    MIL << "Adding '" << catalog << "' as zypp owned." << endl;
    zyppOwnedCatalogs( &catalogs );

    return;
}

// remove catalog from zypp owned (-> service-delete)
void
removeZyppOwned( std::string catalog )
{
    StringList catalogs( zyppOwnedCatalogs( ) );
    catalogs.remove ( catalog );
    MIL << "Removing '" << catalog << "' from zypp owned." << endl;
    zyppOwnedCatalogs( &catalogs );
    return;
}

}; // namespace backend

// EOF
