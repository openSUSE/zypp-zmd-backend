
#include <sqlite3.h>
#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "locks"

#include "dbsource/DbAccess.h"

#include "zypp/base/Logger.h"
#include "zypp/PoolItem.h"
#include "zypp/CapFactory.h"
#include "zypp/CapMatchHelper.h"


#include "locks.h"

using namespace std;
using namespace zypp;

bool
lockItem ( const CapAndItem &cai_r )
{
  MIL << "Locking " << cai_r.item << "(matched by " << cai_r.cap << ")" << endl;
  PoolItem_Ref item(cai_r.item);
  item.status().setLock( true, ResStatus::USER);
  return true;
}

int
read_locks (const ResPool & pool, sqlite3 *db)
{
    MIL << "read_locks" << endl;

    sqlite3_stmt *handle = NULL;

    const char  *sql = "SELECT id, name, version, release, epoch, arch, relation, catalog, glob, importance, importance_gteq FROM locks";
    int rc = sqlite3_prepare (db, sql, -1, &handle, NULL);
    if (rc != SQLITE_OK)
    {
      ERR << "Can not prepare locks selection clause: " << sqlite3_errmsg (db) << endl;
        return -1;
    }

    int count = 0;

    CapFactory cap_factory;
    
    while ((rc = sqlite3_step (handle)) == SQLITE_ROW)
    { 
      string name_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 1));
      string version_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 2));
      string release_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 3));
      int epoch = sqlite3_column_int( handle, 4);
      int arch_id = sqlite3_column_int( handle, 5);
      RCResolvableRelation rc_rel = (RCResolvableRelation) sqlite3_column_int( handle, 6);
      string catalog_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 7));
      string glob_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 8));
      int importance = sqlite3_column_int( handle, 9);
      int  importance_gteq = sqlite3_column_int( handle, 10);
      
      Capability capability = cap_factory.parse( ResTraits<zypp::Package>::kind, name_str, DbAccess::Rc2Rel(rc_rel), Edition(version_str, release_str, epoch) );
      forEachMatchIn( pool, Dep::PROVIDES, capability, lockItem );

      ++count;
    }

    sqlite3_finalize (handle);

    if (rc != SQLITE_DONE)
    {
      ERR << "Error reading lock packages: " << sqlite3_errmsg (db) << endl;
      return -1;
    }

    return count;
}

