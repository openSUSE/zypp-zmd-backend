/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <iostream>

#include "dbsource/zmd-backend.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/SourceManager.h>
#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>
#include <zypp/base/Algorithm.h>

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "query-pool"

#include "KeyRingCallbacks.h"

using namespace std;
using namespace zypp;

//-----------------------------------------------------------------------------

class PrintItem : public resfilter::PoolItemFilterFunctor
{
public:
  const string & _catalog;

  PrintItem( const string & catalog )
      : _catalog( catalog )
  { }

  bool operator()( PoolItem_Ref item )
  {
    if (_catalog.empty()
        || _catalog == item->source().alias())
    {
      cout << (item.status().isInstalled() ? "i" : " ") << "|";
      cout << item->kind() << "|";
      cout << item->name() << "|";
      cout << item->edition().version();
      if (!item->edition().release().empty())
        cout << "-" << item->edition().release();
      cout << "|";
      cout << item->arch() << endl;
    }
    return true;
  }
};




static void
query_pool( ZYpp::Ptr Z, const string & filter, const string & catalog)
{
  Resolvable::Kind kind;

#define FILTER_ALL "all"

  if (filter == "packages") kind = ResTraits<zypp::Package>::kind;
  else if (filter == "patches") kind = ResTraits<zypp::Patch>::kind;
  else if (filter == "patterns") kind = ResTraits<zypp::Pattern>::kind;
  else if (filter == "selections") kind = ResTraits<zypp::Selection>::kind;
  else if (filter == "products") kind = ResTraits<zypp::Product>::kind;
  else if (!filter.empty() && filter != FILTER_ALL)
  {
    std::cerr << "usage: query-pool [packages|patches|patterns|products] [<alias>]" << endl;
    exit( 1 );
  }

  bool system = (catalog == "@system");

  MIL << "query_pool kind '" << kind << "', catalog '" << catalog << "'" << endl;

  SourceManager_Ptr manager = SourceManager::sourceManager();

  if (!system)
  {
    try
    {
      manager->restore( "/" );
    }
    catch (Exception & excpt_r)
    {
      ZYPP_CAUGHT( excpt_r );
      ERR << "Couldn't restore sources" << endl;
      exit( 1 );
    }
  }
    
  // add resolvables from the system
  if ( filter != FILTER_ALL )
  {
    ResStore items;
    for (ResStore::resfilter_const_iterator it = Z->target()->byKindBegin(kind); it != Z->target()->byKindEnd(kind); ++it)
    {
      items.insert(*it);
    }
    Z->addResolvables( items, true );
  }
  else
  {
    // no filter, just add themm all
    Z->addResolvables( Z->target()->resolvables(), true );
  }

  // add all non-installed (cached sources) resolvables to the pool
  // remark: If only the systems resolvables should be shown (catalog == "@system")
  //         then the SourceManager is not initialized (see approx. 20 lines above)
  //         and the following loop is not run at all.

  for (SourceManager::Source_const_iterator it = manager->Source_begin(); it !=  manager->Source_end(); ++it)
  {
    zypp::ResStore store = it->resolvables();
    MIL << "Catalog " << it->id() << " contributing " << store.size() << " resolvables" << endl;
    Z->addResolvables( store, false );
  }

  MIL << "Pool has " << Z->pool().size() << " entries" << endl;

  if (filter.empty()
      || filter == FILTER_ALL)
  {
    PrintItem printitem( system ? "" : catalog );
    if (system)
      zypp::invokeOnEach( Z->pool().begin(), Z->pool().end(),				// all kinds
                          zypp::resfilter::ByInstalled(),
                          zypp::functor::functorRef<bool,PoolItem> (printitem) );
    else
      zypp::invokeOnEach( Z->pool().begin(), Z->pool().end(),				// all kinds
                          zypp::functor::functorRef<bool,PoolItem> (printitem) );

  }
  else
  {
    PrintItem printitem( system ? "" : catalog );
    if (system)
      zypp::invokeOnEach( Z->pool().byKindBegin( kind ), Z->pool().byKindEnd( kind ),	// filter kind
                          zypp::resfilter::ByInstalled(),
                          zypp::functor::functorRef<bool,PoolItem> (printitem) );
    else
      zypp::invokeOnEach( Z->pool().byKindBegin( kind ), Z->pool().byKindEnd( kind ),	// filter kind
                          zypp::functor::functorRef<bool,PoolItem> (printitem) );
  }
  return;
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
  string filter;
  if (argc > 1)
    filter = argv[1];
  string catalog;
  if (argc > 2)
    catalog = argv[2];

  MIL << "START query-pool " << filter << " " << catalog << endl;

  ZYpp::Ptr Z = backend::getZYpp( true );
  KeyRingCallbacks keyring_callbacks;
  DigestCallbacks digest_callbacks;

  Target_Ptr target = backend::initTarget( Z, "/" );

  query_pool( Z, filter, catalog );

  MIL << "END query-pool, result 0" << endl;

  return 0;
}
