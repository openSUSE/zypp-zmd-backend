/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <iostream>

#include "zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/SourceManager.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>

using namespace std;
using namespace zypp;

#include <sqlite3.h>
#include "dbsource/DbAccess.h"
#include "KeyRingCallbacks.h"

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "query-system"

//-----------------------------------------------------------------------------

int
main (int argc, char **argv)
{
    if (argc != 2) {
	std::cerr << "usage: " << argv[0] << " <database>" << endl;
	return 1;
    }

    const char *logfile = getenv("ZYPP_LOGFILE");
    if (logfile != NULL)
	zypp::base::LogControl::instance().logfile( logfile );
    else
	zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

    MIL << "-------------------------------------" << endl;
    MIL << "START query-system " << argv[1] << endl;

    ZYpp::Ptr God = backend::getZYpp( true );
    KeyRingCallbacks keyring_callbacks;
    DigestCallbacks digest_callbacks;

    Target_Ptr target = backend::initTarget( God, false );

    DbAccess db( argv[1] );
    if (!db.openDb( true )) {
	return 1;
    }

    db.writeStore( God->target()->resolvables(), ResStatus::installed, "@system", ZYPP_OWNED );

    db.closeDb();

    MIL << "END query-system, result 0" << endl;

    return 0;
}
