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
#include <zypp/CapMatchHelper.h>

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
complain_if_locked( PoolItem_Ref item, PackageOpType action )
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

  //cerr << "1|" << item_to_string( item ) << " is locked and cannot be " << action_s << endl;
  ERR << item << " is locked and cannot be " << action_s << " (ignoring in transaction)" << endl;
  return false;
}


struct CopyTransaction
{
  ResObject::constPtr _obj;
  PackageOpType _action;
  PoolItem_Ref affected;
  // additional requires
  ResPool::AdditionalCapSet _aCapSet;
  
  CopyTransaction( ResObject::constPtr obj, PackageOpType action, ResPool::AdditionalCapSet aCapSet )
      : _obj( obj )
      , _action( action )
      , _aCapSet(aCapSet)
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
          return complain_if_locked( item, _action );
        }
	else
          item.status().setToBeUninstalled( ResStatus::APPL_HIGH );
        break;
      case PACKAGE_OP_INSTALL:
        if (check_lock( item ))
        {
          return complain_if_locked( item, _action );
        }
	else
          item.status().setToBeInstalled( ResStatus::APPL_HIGH );
        break;
      case PACKAGE_OP_UPGRADE:
        if (check_lock( item ))
        {
          return complain_if_locked( item, _action );
        }
	else
	{
          item.status().setToBeInstalled( ResStatus::APPL_HIGH );
	}
        break;
      case PACKAGE_OP_INSTALL_BEST:
        pool_install_best( item.resolvable()->kind(), item.resolvable()->name() );
	return true;
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

  struct GreaterInstalledEdition
  {
    GreaterInstalledEdition()
	: installed_count(0)
    {}

    bool operator()( const CapAndItem &cai_r )
    {
      PoolItem_Ref item(cai_r.item);
      if ( item.status().isInstalled() )
      {
	  installed_count++;
	  // found an installed item
	  if ( item->edition() >= edition )
	  {
	      MIL << "installed " << item << " is newer" << endl;
	      edition = item->edition();
	  }
      }
      return true;
    }

    Edition edition;
    bool installed_count;
  };

  
  void
  pool_install_best( const Resolvable::Kind &kind, const std::string &name )
  {
      if ( name.empty() )
      {
	cerr << "1|" << "Got empty requirement." << endl;
	exit(1);
      }
      
     // The user is setting this capablility
      Capability cap = CapFactory().parse( kind, name );

      // if we have a installed item, we want to inject a requirement that
      // see what is the greatest installed edition
      GreaterInstalledEdition gieftor;
      
      forEachMatchIn( getZYpp()->pool(), Dep::PROVIDES, cap, functor::functorRef<bool, const CapAndItem &>(gieftor) );
      if ( gieftor.installed_count > 0 )
      {
	  // only add requirement > installed edition if the given capability has
	  // no relation already
	  if ( cap.op() == Rel::NONE )
	  {
	      // recreate the capability to be > gratest installed edition so
	      // the requirement dont say "satisfied" if an old version is installed
	      cap = CapFactory().parse( kind, name, Rel::GT, gieftor.edition );
	  }	  
      }
     
     _aCapSet[ResStatus::APPL_HIGH].insert (cap);
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

  ResPool::AdditionalCapSet aCapSet;
  
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

    CopyTransaction info( obj, action, aCapSet );
    
    invokeOnEach( pool.byNameBegin( obj->name() ),
                  pool.byNameEnd( obj->name() ),
                  functor::functorRef<bool,PoolItem> (info) );
    aCapSet = info._aCapSet;
    
    MIL << "Additional requirements (USER): " << info._aCapSet[ResStatus::USER] << std::endl;
    MIL << "Additional requirements (APPL_HIGH): " << info._aCapSet[ResStatus::APPL_HIGH] << std::endl;
    
    // wth is this?
    //if (info.locked)
    //  return -1;

    if (info.affected)
    {
      items[id] = info.affected;
      DBG << "#" << id << ": " << info.affected << endl;
    }
    else
      ERR << "Item matching id " << id << " not found" << endl;

    ++count;
  }

  // set pool additional requires
  getZYpp()->pool().setAdditionalRequire( aCapSet );

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


// functor to find same item
struct DuplicateFinder
{
  PoolItem_Ref _item;
  bool _found;
 
  DuplicateFinder(PoolItem_Ref item) 
    : _item(item), _found(false)
  {}

  bool operator()( PoolItem_Ref pi )
  {
    if ( compareByNVRA( pi.resolvable(), _item.resolvable()) == 0 )
    {
      _found = true;
      return false;
    }
    return true;
  }
};

bool
write_resobject_set( sqlite3_stmt *handle, const PoolItemSet & objects, PackageOpType op_type, ResolverContext_Ptr context)
{
  if (context == NULL)
  {
    MIL << "Nothing to transact" << endl;
    return true;
  }

  int rc = SQLITE_DONE;

  if ( objects.size() == 0 )
  {
      WAR << "nothing to write, empty transaction set" << endl;
  }
  

  for (PoolItemSet::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
  {

    PoolItem item = *iter;

    if ( check_lock(item) )
    {
      cerr << "1|" << item_to_string( item ) << " is locked." << endl;
      WAR << "Skipping " << item << " (locked)" << endl;
      continue;
    }

    // if the item will be installed, make sure the same item
    // is not already installed
    if ( op_type == PACKAGE_OP_INSTALL )
    {
       DuplicateFinder dup(item);
       zypp::invokeOnEach( getZYpp()->pool().begin(), getZYpp()->pool().end(),
                           zypp::resfilter::ByInstalled(),
                           zypp::functor::functorRef<bool,PoolItem> (dup) );
       if ( dup._found )
       {
         WAR << "Skipping already installed" << item << endl;
         continue;
       }
    }

    // only write those items back which were set by solver (#154976)
    //if (!item.status().isBySolver())
    //{
    //  DBG << "Skipping " << item << endl;
    //  continue;
    //}

    string details = dep_get_package_info( context, *iter );

    sqlite3_bind_int( handle, 1, (int) op_type );
    sqlite3_bind_int( handle, 2, iter->resolvable()->zmdid() );
    sqlite3_bind_text( handle, 3, details.c_str(), -1, SQLITE_STATIC );
    MIL << "writing transacton for " << *iter << endl;
    
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

  // start a transaction
  sqlite3_exec(db, "begin;", NULL, NULL, NULL);
  // drop all transactions
  sqlite3_exec(db, "delete from transactions;", NULL, NULL, NULL);

  if (context == NULL)
  {
    MIL << "Nothing to transact" << endl;
    sqlite3_exec(db, "commit;", NULL, NULL, NULL);
    return true;
  }

  sqlite3_stmt *handle = NULL;
  const char *sql = "INSERT INTO transactions (action, id, details) VALUES (?, ?, ?)";
  int rc = sqlite3_prepare( db, sql, -1, &handle, NULL );
  if (rc != SQLITE_OK)
  {
    ERR << "Can not prepare transaction insertion clause: " << sqlite3_errmsg (db) << endl;
    sqlite3_exec(db, "rollback;", NULL, NULL, NULL);
    return false;
  }
  PoolItemSet install_set;
  PoolItemSet remove_set;

  context->foreachInstall( insert_item, &install_set);
  MIL << "Items to install:" << endl;
  print_set( install_set );
  context->foreachUninstall( insert_item, &remove_set);
  MIL << "Items to uninstall:" << endl;
  print_set( remove_set );
  context->foreachUpgrade( insert_item_pair, &install_set);
  MIL << "Items to upgrade:" << endl;
  print_set( install_set );

  MIL << "now writing back to database..." << endl;  

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

  // commit the result
  sqlite3_exec(db, "commit;", NULL, NULL, NULL);
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
