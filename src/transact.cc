/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <cstdlib>	// setenv
#include <iostream>
#include <string>
#include <list>

#include "dbsource/zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>
#include <zypp/media/MediaException.h>

#include <zypp/ExternalProgram.h>

using namespace std;
using namespace zypp;

#include <sqlite3.h>
#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "transact"

#include "dbsource/utils.h"
#include "dbsource/DbAccess.h"
#include "dbsource/DbSources.h"

#include "transactions.h"
#include <zypp/solver/detail/ResolverInfo.h>

#include "RpmCallbacks.h"
#include "MediaChangeCallback.h"
#include "MessageResolvableReportCallback.h"
#include "KeyRingCallbacks.h"

using solver::detail::ResolverInfo_Ptr;

typedef std::list<PoolItem> PoolItemList;

//-----------------------------------------------------------------------------

static void
append_dep_info (ResolverInfo_Ptr info, void *user_data)
{
  string *dep_failure_info = (string *)user_data;
  bool debug = false;

  if (info == NULL)
  {
    ERR << "append_dep_info(NULL)" << endl;
    return;
  }

  if (getenv ("RCD_DEBUG_DEPS"))
    debug = true;

  if (debug || info->important())
  {
    dep_failure_info->append( "\n" );
    if (debug && info->error())
      dep_failure_info->append( "ERR " );
    if (debug && info->important())
      dep_failure_info->append( "IMP " );
    dep_failure_info->append( info->message() );
  }
  return;
}


static void
drop_transacted( sqlite3 *db, IdItemMap & items )
{
  for (IdItemMap::const_iterator it = items.begin(); it != items.end(); ++it)
  {
    if (it->second.status().transacts())
    {			// transaction still set -> package was not committed yet
      continue;
    }
    drop_transaction( db, it->first );
  }
  return;
}


int
main (int argc, char **argv)
{
  if (argc < 2)
  {
    cerr << "1|Usage: " << argv[0] << " <database> [--test] [--nosignature] [--nopretest]" << endl;
    return 1;
  }

  int argp = 2;

  bool dry_run = false;
  bool nosignature = false;
  bool nopretest = false;
  while (argp < argc)
  {
    string arg(argv[argp]);
    if (arg == "--test") dry_run = true;
    if (arg == "--nosignature") nosignature = true;
#warning nopretest not honored
    if (arg == "--nopretest") nopretest = true;
    argp++;
  }

  const char *logfile = getenv("ZYPP_LOGFILE");
  if (logfile != NULL)
    zypp::base::LogControl::instance().logfile( logfile );
  else
    zypp::base::LogControl::instance().logfile( ZMD_BACKEND_LOG );

  MIL << "-------------------------------------" << endl;
  MIL << "START transact " << argv[1] << (dry_run?" --test":" ") << (nosignature?" --nosignature":"") <<  endl;

  // access the sqlite db

  DbAccess db (argv[1]);
  if (!db.openDb(false))
    return 1;

  // start ZYPP

  ZYpp::Ptr God = backend::getZYpp();
  KeyRingCallbacks keyring_callbacks;
  DigestCallbacks digest_callbacks;

  Target_Ptr target = backend::initTarget( God );
  target->enableStorage( "/" );			// transact affects the target store

  // load the catalogs and resolvables from sqlite db

  DbSources dbs(db.db());

  const SourcesList & sources = dbs.sources( true );	// create actual zypp sources

  for (SourcesList::const_iterator it = sources.begin(); it != sources.end(); ++it)
  {
    zypp::ResStore store = it->resolvables();
    MIL << "Catalog " << it->id() << ", type " << it->type() << " contributing " << store.size() << " resolvables" << endl;
    God->addResolvables( store, (it->id() == "@system") );
  }

  // now the pool is complete, add transactions

  IdItemMap items;

  int removals = 0;
  int count = read_transactions (God->pool(), db.db(), dbs, removals, items);
  if (count < 0)
  {
    cerr << "1|Reading transactions failed." << endl;
    return 1;
  }

  // total number of steps (*2, one for start/preparing, one for install/deleting)
  cout << "0|" << count*2 << endl;

  if (count == 0)
  {
    MIL << "No transactions found" << endl;
    cout << "4" << endl;
    return 0;
  }

  RpmCallbacks rpm_callbacks;				// init and connect rpm progress callbacks
  MediaChangeCallback med_callback;			// init and connect media change callback
  MessageResolvableReportCallback msg_callback;	// init and connect patch message callback

  int result = 0;

  try
  {
    PoolItemList x,y,z;

    ::setenv( "YAST_IS_RUNNING", "1", 1 );

    ZYppCommitPolicy policy;
    if (dry_run) policy.dryRun( true );
    if (nosignature) policy.rpmNoSignature( true );

    // we dont need to reload rpm database after commit
    myPolicy.syncPoolAfterCommit( false );
    
    ZYppCommitResult zres = God->commit( policy );

    // removals aren't counted in zres._result, only installs are
    // so check if there where installs at all before checking zres._result

    if (((count - removals > 0)				// any installs ?
         && (zres._result <= 0))				// but no items committed ?
        || zres._errors.size() > 0)				// or any errors
    {
      result = 1;
      if (zres._errors.size() > 0)
      {

        // write transaction progress to stdout

        cout << "3|Incomplete transactions:" << endl;
        for (PoolItemList::const_iterator it = zres._errors.begin(); it != zres._errors.end(); ++it)
        {
          cout << "3|" << it->resolvable() << endl;
        }
      }
    }

    if (!dry_run)
    {
      ExternalProgram suseconfig( "/sbin/SuSEconfig", ExternalProgram::Discard_Stderr );	// should redirect stderr to logfile
      suseconfig.close();		// discard exit code
    }
  }
  catch ( const media::MediaException & expt_r )
  {
    ZYPP_CAUGHT( expt_r );
    result = 1;
    if (med_callback.mediaNr() != 0			// exception due to MediaChange callback ?
        && !med_callback.description().empty())
    {
      // "4|" means 'message' to ZMD
      cerr << "4|Need media " << med_callback.mediaNr() << ": " << med_callback.description() << endl;
    }
    else
    {
      // "3|" is progress to stdout
      cout << "3|" << joinlines( expt_r.asUserString() ) << endl;
      // report as message ("4|" is message) to stderr
      cerr << "4|" << expt_r.asString() << endl;
    }
  }
  catch ( const Exception & expt_r )
  {
    ZYPP_CAUGHT( expt_r );
    result = 1;
    // transact progress to stdout
    cout << "3|" << joinlines( expt_r.asUserString() ) << endl;
    // report as error
    cerr << "1|" << expt_r.asString() << endl;
  }

  // 'finish' transaction progress
  cout << "4" << endl;

  // now drop those transactions which are already committed

  drop_transacted( db.db(), items );

  db.closeDb();

  MIL << "END transact, result " << result << endl;

  if (count > 1
      && dry_run)
  {
    return 0;	// bug #164583
  }
  return result;
}
