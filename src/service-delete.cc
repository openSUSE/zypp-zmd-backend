/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <iostream>

#include "zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/SourceManager.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>
#include <zypp/base/Algorithm.h>

using namespace std;
using namespace zypp;

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "service-delete"

//-----------------------------------------------------------------------------

static int
service_delete( ZYpp::Ptr Z, const string & uri)
{
    SourceManager_Ptr manager = SourceManager::sourceManager();

    MIL << "service_delete(" << uri << ")" << endl;

    try {
	manager->restore( "/" );
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT( excpt_r );
	ERR << "Couldn't restore sources" << endl;
	return 1;
    }

    SourceManager::Source_const_iterator it;
    for (it = manager->Source_begin(); it !=  manager->Source_end(); ++it) {
	string src_uri = it->url().asString() + "?alias=" + it->alias();
	MIL << "Uri " << src_uri << endl;
	if (src_uri == uri) {
	    manager->removeSource( it->alias() );
	    try {
 		manager->store ("/", true /*metadata_cache*/);
	    }
	    catch (Exception & excpt_r) {
		ZYPP_CAUGHT (excpt_r);
		ERR << "Couldn't update source cache" << endl;
	    }
	    break;
	}
    }
    if (it == manager->Source_end()) {
	WAR << "Source not found" << endl;
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
    string alias;
    if (argc < 2) {
	cerr << "Usage: service-delete <uri>" << endl;
	exit( 1 );
    }
    alias = argv[1];

    MIL << "START service-delete " << alias << endl;

    ZYpp::Ptr Z = backend::getZYpp( true );
    Target_Ptr target = backend::initTarget( Z, false );

    int result = service_delete( Z, alias );

    MIL << "END service-delete" << endl;

    return result;
}
