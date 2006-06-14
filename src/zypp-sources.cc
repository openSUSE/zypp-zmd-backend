/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
//
// zypp-sources.cc
//
// debug helper to print all sources known to zypp
//

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
#define ZYPP_BASE_LOGGER_LOGGROUP "zypp-sources"

#include <sqlite3.h>
#include <cstring>

#include <sys/stat.h>

#include "dbsource/DbAccess.h"
#include "KeyRingCallbacks.h"

//----------------------------------------------------------------------------
static SourceManager_Ptr manager;

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

int
main (int argc, char **argv)
{
    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    ZYpp::Ptr God = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    backend::initTarget( God );

    manager = SourceManager::sourceManager();
    if (! restore_sources ())
	return 1;

    SourceManager::Source_const_iterator it;
    int count = 0;
    for (it = manager->Source_begin(); it !=  manager->Source_end(); ++it) {
	cout << ++count << ". Url '" << it->url().asString() << "', Alias '" << it->alias() << "'" << endl;
    }
    if (count == 0)
	cout << "No zypp sources found" << endl;
    return 0;
}
