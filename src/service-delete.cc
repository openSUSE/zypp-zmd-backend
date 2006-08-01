/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include "dbsource/zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/SourceManager.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>
#include <zypp/base/Algorithm.h>

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "service-delete"

#include "dbsource/utils.h"
#include "KeyRingCallbacks.h"

using namespace std;
using namespace zypp;

//-----------------------------------------------------------------------------

static int
service_delete( ZYpp::Ptr Z, const string & name)
{
    SourceManager_Ptr manager = SourceManager::sourceManager();

    MIL << "service_delete(" << name << ")" << endl;

    Url uri;
    url::ParamMap uriparams;
    string urialias;

    // Url() constructor might throw
    try {
	uri = Url ( (name[0] == '/') ? (string("file:") + name) : name );
	uriparams = uri.getQueryStringMap();	// extract parameters
	url::ParamMap::const_iterator it = uriparams.find( "alias" );
	if (it != uriparams.end()) {
	    urialias = it->second;
	}
    }
    catch ( const Exception & excpt_r ) {
	ZYPP_CAUGHT( excpt_r );
	cerr << "1|" << joinlines( excpt_r.asUserString() ) << endl;
	return 1;
    }

    Source_Ref source = backend::findSource( manager, urialias, uri );

    if (source) {

	manager->removeSource( source.numericId() );
	try {
 	    manager->store ("/", true /*metadata_cache*/);
	}
	catch (Exception & excpt_r) {
	    cerr << "2|Can't remove source '" << name << "': " << joinlines( excpt_r.asUserString() ) << endl;
	    ZYPP_CAUGHT (excpt_r);
	    ERR << "Couldn't update source cache" << endl;
	    return 1;
	}
    }
    else {
	cerr << "3|Source '" << name << "' already removed." << endl;
	WAR << "Source '" << name << "' not found" << endl;
	return 1;
    }
    return 0;
}

//-----------------------------------------------------------------------------

int
main (int argc, char **argv)
{
    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    if (argc < 3) {
	cerr << "Usage: service-delete <db> <uri>" << endl;
	exit( 1 );
    }
    string db( argv[1] );
    string name( argv[2] );

    MIL << "START service-delete " << db << " " << name << endl;

    // if its listed as zypp owned, remove it
    backend::removeZyppOwned( name );

    struct stat st;
    if (stat( "/var/lib/zypp/sources-being-processed-by-yast", &st ) == 0) {
	MIL << "Processed by YaST, exiting" << endl;
	return 0;
    }

    ZYpp::Ptr Z = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    Target_Ptr target = backend::initTarget( Z );

    int result = service_delete( Z, name );

    MIL << "END service-delete, result " << result << endl;

    return result;
}
