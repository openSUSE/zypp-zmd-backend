/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <iostream>
#include <string>

#include "dbsource/zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/VendorAttr.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>

using namespace std;
using namespace zypp;

#include <sqlite3.h>
#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "resolve-dependencies"

#include "dbsource/DbAccess.h"
#include "dbsource/DbSources.h"
#include "KeyRingCallbacks.h"

#include "transactions.h"
#include "locks.h"
#include <zypp/solver/detail/ResolverInfo.h>

using solver::detail::ResolverInfo_Ptr;

//-----------------------------------------------------------------------------

int
main (int argc, char **argv)
{
  if (argc < 3)
  {
    cerr << "usage: " << argv[0] << " zmd-db test-case-output-path" << endl;
    return 1;
  }

  const char *logfile = getenv("ZYPP_LOGFILE");
  if (logfile != NULL)
    zypp::base::LogControl::instance().logfile( logfile );
  else
    zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

  MIL << "-------------------------------------" << endl;
  MIL << "START zmd-solver-testcase " << argv[1] << endl;

  // access the sqlite db

  DbAccess db (argv[1]);
  if (!db.openDb(false))
    return 1;

  // we honor zmd locks, so disable autoprotecton of
  // foreign vendors
  zypp::VendorAttr::disableAutoProtect();

  ZYpp::Ptr God = backend::getZYpp( true );
  KeyRingCallbacks keyring_callbacks;
  DigestCallbacks digest_callbacks;

  Target_Ptr target = backend::initTarget( God );

  // load the catalogs and resolvables from sqlite db

  DbSources dbs(db.db());

  const SourcesList & sources = dbs.sources();

  for (SourcesList::const_iterator it = sources.begin(); it != sources.end(); ++it)
  {
    zypp::ResStore store = it->resolvables();
    MIL << "Catalog " << it->id() << " contributing " << store.size() << " resolvables" << endl;
    God->addResolvables( store, (it->id() == "@system") );
  }

  // update-status is supposed to do this
  // but resolvables dont have a status yet
  God->resolver()->establishPool();

  // now the pool is complete, add transactions

  // read locks first
  read_locks (God->pool(), db.db());

  int removals = 0; // unused here
  IdItemMap transacted_items; // unused here

  bool have_best_package = false;

  int count = read_transactions( God->pool(), db.db(), dbs, removals, transacted_items, have_best_package );
  if (count < 0)
    return 1;

  MIL << "Processing " << count << " transactions" << endl;
  bool success = true;
  God->resolver()->createSolverTestcase ( argv[2] );

  db.closeDb();

  MIL << "END zmd-testcase, result " << (success ? 0 : 1) << endl;

  return (success ? 0 : 1);
}
