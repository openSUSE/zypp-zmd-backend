/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/** file: zmd/backend/transactions.cc
    read/write 'transactions' table to/from ResPool
 */

#include <set>
#include <sstream>
#include "transactions.h"

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>

#include <zypp/base/Logger.h>
#include <zypp/base/Exception.h>
#include <zypp/base/Algorithm.h>

#include <zypp/CapFactory.h>
#include <zypp/ResPool.h>
#include <zypp/ResFilters.h>
#include <zypp/CapFilters.h>

#include <zypp/solver/detail/ResolverContext.h>
#include <zypp/solver/detail/ResolverInfo.h>

using std::endl;
using namespace zypp;

#include <sqlite3.h>
#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "transactions"

#include "dbsource/DbAccess.h"
#include "dbsource/DbSources.h"

typedef enum {
  /**
   * This operations means removing a resolvable
   * (Installed)
   */
  PACKAGE_OP_REMOVE  = 0,
  /**
   * This operations means intalling a resolvable
   * by specific version or the lastest available
   */
  PACKAGE_OP_INSTALL = 1,
  /**
   * Currently unused
   */
  PACKAGE_OP_UPGRADE = 2,
  /**
   * This operations means intalling a resolvable
   * by specifiying a provides.
   */
  PACKAGE_OP_INSTALL_BEST = 3
} PackageOpType;


using namespace std;
using namespace zypp;
using solver::detail::ResolverInfo_Ptr;
using solver::detail::ResolverContext_Ptr;

typedef std::set<PoolItem> PoolItemSet;

//-----------------------------------------------------------------------------

static void
pool_install_best( const Resolvable::Kind &kind, const std::string &name )
{
  // as documented in ResPool::setAdditionalFoo
  CapSet capset;
  capset.insert (CapFactory().parse( kind, name));
  // The user is setting this capablility
  ResPool::AdditionalCapSet aCapSet;
  aCapSet[ResStatus::USER] = capset;
  getZYpp()->pool().setAdditionalRequire( aCapSet );
  //item.status().setToBeInstalled( ResStatus::USER );
}


// check if pool item is locked
static bool
check_lock( PoolItem_Ref item )
{
  return item.status().isLocked();
}


static string
item_to_string( PoolItem_Ref item )
{
  ostringstream os;
  if (!item) return "";

  if (item->kind() != ResTraits<zypp::Package>::kind)
    os << item->kind() << ':';
  os  << item->name() << '-' << item->edition();
  if (item->arch() != "")
  {
    os << '.' << item->arch();
  }
  return os.str();
}

// complain about locked pool item
static bool
is_locked( PoolItem_Ref item, PackageOpType action )
{
  char *action_s = "";
  switch ( action )
  {
  case PACKAGE_OP_REMOVE:
    action_s = "removed";
    break;
  case PACKAGE_OP_INSTALL:
    action_s = "installed";
    break;
  case PACKAGE_OP_UPGRADE:
    action_s = "upgraded";
    break;
  default:
    return true;	// don't honor locks on undefined actions
  }

  cerr << "1|" << item_to_string( item ) << " is locked and cannot be " << action_s << endl;
  ERR << item << " is locked and cannot be " << action_s << endl;
  return false;
}


struct CopyTransaction
{
  ResObject::constPtr _obj;
  PackageOpType _action;
  PoolItem_Ref affected;
  bool locked;

  CopyTransaction( ResObject::constPtr obj, PackageOpType action )
      : _obj( obj )
      , _action( action )
      , locked( false )
  { }

  bool operator()( PoolItem_Ref item )
  {
    if (item.resolvable() == _obj)
    {
      switch (_action)
      {
      case PACKAGE_OP_REMOVE:
        if (check_lock( item ))
        {
          locked = true;
          return is_locked( item, _action );
        }
        item.status().setToBeUninstalled( ResStatus::USER );
        break;
      case PACKAGE_OP_INSTALL:
        if (check_lock( item ))
        {
          locked = true;
          return is_locked( item, _action );
        }
        item.status().setToBeInstalled( ResStatus::USER );
        break;
      case PACKAGE_OP_UPGRADE:
        if (check_lock( item ))
        {
          locked = true;
          return is_locked( item, _action );
        }
        item.status().setToBeInstalled( ResStatus::USER );
        break;
      case PACKAGE_OP_INSTALL_BEST:
        pool_install_best( item.resolvable()->kind(), item.resolvable()->name() );
        break;
      default:
        ERR << "Ignoring unknown action " << _action << endl;
        break;
      }
      affected = item;
      return false;			// stop looking
    }
    return true;		// continue looking
  }
};

//
// read all transactions from the transactions table and
//  set the appropriate status in the pool
// return -1 on error
//  else the number of transactions (installs, removals, and upgrades)
// return the number of removals in removals

int
read_transactions (const ResPool & pool, sqlite3 *db, const DbSources & sources, int & removals, IdItemMap & items, bool & have_best_package )
{
  MIL << "read_transactions" << endl;

  sqlite3_stmt *handle = NULL;
  const char  *sql = "SELECT action, id FROM transactions";
  int rc = sqlite3_prepare (db, sql, -1, &handle, NULL);
  if (rc != SQLITE_OK)
  {
    ERR << "Can not prepare transaction selection clause: " << sqlite3_errmsg (db) << endl;
    return -1;
  }

  int count = 0;

  removals = 0;

  while ((rc = sqlite3_step (handle)) == SQLITE_ROW)
  {
    int id;
    PackageOpType action;
    ResObject::constPtr obj;

    action = (PackageOpType) sqlite3_column_int( handle, 0 );
    id = sqlite3_column_int( handle, 1 );					// get the id

    obj = sources.getById( id );						// get the ResObject by Id
    if (obj == NULL)
    {
      ERR << "Can't find res object id " << id << endl;
      cerr << "1|Resolvable id " << id << " does not exist." << endl;
      break;
    }

    if (action == PACKAGE_OP_REMOVE)
      ++removals;

    if (action == PACKAGE_OP_INSTALL_BEST)
      have_best_package = true;

    // now we have the ResObject, but for setting the status we need
    //  the corresponding PoolItem
    // So loop over the pool (over items with the same name)
    //  and find the PoolItem which refers to the ResObject

    CopyTransaction info( obj, action );

    invokeOnEach( pool.byNameBegin( obj->name() ),
                  pool.byNameEnd( obj->name() ),
                  functor::functorRef<bool,PoolItem> (info) );

    if (info.locked)
      return -1;

    if (info.affected)
    {
      items[id] = info.affected;
      DBG << "#" << id << ": " << info.affected << endl;
    }
    else
      ERR << "Item matching id " << id << " not found" << endl;

    ++count;
  }

  sqlite3_finalize (handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error reading transaction packages: " << sqlite3_errmsg (db) << endl;
    return -1;
  }

  return count;
}

//-----------------------------------------------------------------------------

static void
insert_item( PoolItem_Ref item, const ResStatus & status, void *data)
{
  PoolItemSet *pis = (PoolItemSet *)data;
  pis->insert( item );
}

static void
insert_item_pair (PoolItem_Ref install, const ResStatus & status1, PoolItem_Ref remove, const ResStatus & status2, void *data)
{
  PoolItemSet *pis = (PoolItemSet *)data;
  pis->insert( install );		// only the install
}



static void
dep_get_package_info_cb (ResolverInfo_Ptr info, void *user_data)
{
  string *msg = (string *)user_data;

  msg->append( info->message() );
  msg->append( "|" );

} /* dep_get_package_info_cb */


static string
dep_get_package_info (ResolverContext_Ptr context, PoolItem_Ref item)
{
  string info;

  context->foreachInfo (item, RESOLVER_INFO_PRIORITY_USER, dep_get_package_info_cb, &info);

  return info;
} /* dep_get_package_info */


bool
write_resobject_set( sqlite3_stmt *handle, const PoolItemSet & objects, PackageOpType op_type, ResolverContext_Ptr context)
{
  int rc = SQLITE_DONE;

  for (PoolItemSet::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
  {

    PoolItem item = *iter;

    // only write those items back which were set by solver (#154976)
    if (!item.status().isBySolver())
    {
      DBG << "Skipping " << item << endl;
      continue;
    }

    string details = dep_get_package_info( context, *iter );

    sqlite3_bind_int( handle, 1, (int) op_type );
    sqlite3_bind_int( handle, 2, iter->resolvable()->zmdid() );
    sqlite3_bind_text( handle, 3, details.c_str(), -1, SQLITE_STATIC );

    rc = sqlite3_step( handle );
    sqlite3_reset( handle );
  }
  return (rc == SQLITE_DONE);
}


static void
print_set( const PoolItemSet & objects )
{
  for (PoolItemSet::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
  {
    MIL << *iter << endl;
  }
}

bool
write_transactions (const ResPool & pool, sqlite3 *db, ResolverContext_Ptr context)
{
  MIL << "write_transactions" << endl;

  sqlite3_stmt *handle = NULL;
  const char *sql = "INSERT INTO transactions (action, id, details) VALUES (?, ?, ?)";
  int rc = sqlite3_prepare( db, sql, -1, &handle, NULL );
  if (rc != SQLITE_OK)
  {
    ERR << "Can not prepare transaction insertion clause: " << sqlite3_errmsg (db) << endl;
    return false;
  }
  PoolItemSet install_set;
  PoolItemSet remove_set;

  context->foreachInstall( insert_item, &install_set);
  MIL << "foreachInstall" << endl;
  print_set( install_set );
  context->foreachUninstall( insert_item, &remove_set);
  MIL << "foreachUninstall" << endl;
  print_set( remove_set );
  context->foreachUpgrade( insert_item_pair, &install_set);
  MIL << "foreachUpgrade" << endl;
  print_set( install_set );

  bool result;
  result = write_resobject_set( handle, install_set, PACKAGE_OP_INSTALL, context );
  if (!result)
  {
    ERR << "Error writing transaction install set: " << sqlite3_errmsg (db) << endl;
  }
  result = write_resobject_set( handle, remove_set, PACKAGE_OP_REMOVE, context );
  if (!result)
  {
    ERR << "Error writing transaction remove set: " << sqlite3_errmsg (db) << endl;
  }

  sqlite3_finalize( handle );

  return result;
}

//-----------------------------------------------------------------------------

// delete transaction with a specific ID
// used by 'transact' helper after package commit()

void
drop_transaction (sqlite3 *db, sqlite_int64 id)
{
  sqlite3_stmt *handle = NULL;
  const char *sql = "DELETE FROM transactions WHERE id = ?";
  int rc = sqlite3_prepare( db, sql, -1, &handle, NULL );
  if (rc != SQLITE_OK)
  {
    ERR << "Can not prepare transaction delete clause: " << sqlite3_errmsg (db) << endl;
    return;
  }

  sqlite3_bind_int64( handle, 1, id );

  rc = sqlite3_step( handle );
  if (rc != SQLITE_DONE)
  {
    ERR << "Can not remove transaction: " << sqlite3_errmsg (db) << endl;
  }

  sqlite3_reset( handle );

  return;
}
