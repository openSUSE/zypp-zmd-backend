//
// locks.cc
//
// read 'locks' table and assign state 'locked:by_user' to pool items
//
//
#include <sqlite3.h>
#include <set>

#include <boost/regex.hpp>
#include <boost/function.hpp>

#include "dbsource/DbAccess.h"

#include "zypp/base/Logger.h"
#include "zypp/PoolItem.h"
#include "zypp/CapFactory.h"
#include "zypp/CapMatchHelper.h"
#include "zypp/capability/Capabilities.h"

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "locks"


#include "locks.h"

using namespace std;
using namespace boost;
using namespace zypp;


//
// collect matching names
//
// called by regexp matching, see 'Match' below
//

struct NameMatchCollectorFunc
{
  set<string> matches;

  bool operator()( const PoolItem &item )
  {
    matches.insert( item.resolvable()->name() );
    return true;
  }
};


// taken from zypper
struct Match
{
  const boost::regex * _regex;

  Match(const boost::regex & regex ) :
    _regex(&regex)
  {}

  bool operator()(const zypp::PoolItem & pi) const
  {
    return
    // match resolvable name
    regex_match(pi.resolvable()->name(), *_regex);
  }
};


string
wildcards2regex(const string & str)
{
  string regexed;

  regex all("\\*"); // regex to search for '*'
  regex one("\\?"); // regex to search for '?'
  string r_all(".*"); // regex equivalent of '*'
  string r_one(".");  // regex equivalent of '?'

  // replace all "*" in input with ".*"
  regexed = regex_replace(str, all, r_all);
  MIL << "wildcards2regex: " << str << " -> " << regexed;

  // replace all "?" in input with "."
  regexed = regex_replace(regexed, one, r_one);
  MIL << " -> " << regexed << endl;

  return regexed;
}


//
// assign Lock to installed pool item
//

struct ItemLockerFunc
{
  ItemLockerFunc( const string lock_str )
    : _lock_str(lock_str)
  {}

  bool operator()( const CapAndItem &cai_r )
  {
    PoolItem_Ref item(cai_r.item);
    if ( item.status().isInstalled() )
    {
      MIL << "Locking installed " << cai_r.item << "(matched by " << _lock_str << ")" << endl;
      item.status().setLock( true, ResStatus::USER);
    }
    return true;
  }

  string _lock_str;
};


//
// read 'locks' table, evaluate 'glob' column, assign locks to pool
//

int
read_locks (const ResPool & pool, sqlite3 *db)
{
    MIL << "read_locks" << endl;

    sqlite3_stmt *handle = NULL;

    //                          0     1        2        3      4     5         6        7     8           9               10
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
      // we dont need this data yet
      //string name_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 1));
      //string version_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 2));
      //string release_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 3));
      //int epoch = sqlite3_column_int( handle, 4);
      //int arch_id = sqlite3_column_int( handle, 5);
      //RCResolvableRelation rc_rel = (RCResolvableRelation) sqlite3_column_int( handle, 6);

      const char *catalog = reinterpret_cast<const char*>(sqlite3_column_text( handle, 7));
      string catalog_str;
      if (catalog)
         catalog_str = catalog;

      string glob_str = reinterpret_cast<const char*>(sqlite3_column_text( handle, 8));
      //int importance = sqlite3_column_int( handle, 9);
      //int  importance_gteq = sqlite3_column_int( handle, 10);

      // zypp does not provide wildcard or regex support in the Capability matching helpers
      // but it still parses the index if it contains wildcards.
      // so we decompose the capability, and keep the op and edition, while, the name
      // is converted to a regexp and matched against all possible names in the pool
      // Then these names are combined with the original edition and relation and we
      // got a new capability for matching wildcard to use with the capability match
      // helpers

      Rel rel;
      Edition edition;
      string name;

      try
      {
        Capability capability = cap_factory.parse( ResTraits<zypp::Package>::kind, glob_str );
        if ( capability::VersionedCap::constPtr vercap = capability::asKind<capability::VersionedCap>(capability) )
        {
          rel = vercap->op();
          edition = vercap->edition();
          name = vercap->index();
        }
        else
        {
          ERR << "Not a versioned capability in: [" << glob_str << "] skipping" << std::endl;
          continue;
        }
      }
      catch ( const Exception &e )
      {
        ERR << "Can't parse capability in: " << glob_str << " (" << e.msg() << ") skipping" << std::endl;
        continue;
      }

      // Operator NONE is not allowed in Capability
      if (rel == Rel::NONE) rel = Rel::ANY;

      NameMatchCollectorFunc nameMatchFunc;

      // regex flags
      unsigned int flags = boost::regex::normal;
      flags |= boost::regex_constants::icase;
      boost::regex reg;

      // create regex object
      string regstr( wildcards2regex( name ) );
      MIL << "regstr '" << regstr << "'" << endl;
      try
      {
        reg.assign( regstr, flags );
      }
      catch (regex_error & e)
      {
        ERR << "locks: " << regstr << " is not a valid regular expression: \"" << e.what() << "\"" << endl;
        ERR << "This is a bug, please file a bug report against libzypp-zmd-backend" << endl;
        // ignore this lock and continue
        continue;
      }

      invokeOnEach( pool.begin(), pool.end(), Match(reg), functor::functorRef<bool, const PoolItem &>(nameMatchFunc) );

      MIL << "Found " << nameMatchFunc.matches.size() << " matches." << endl;

      // now we have all the names matching

      // for each name matching try to match a capability

      ItemLockerFunc lockItemFunc( glob_str );

      for ( set<string>::const_iterator it = nameMatchFunc.matches.begin(); it != nameMatchFunc.matches.end(); ++it )
      {
        string matched_name = *it;

        try
        {
          Capability capability = cap_factory.parse( ResTraits<zypp::Package>::kind, matched_name, rel, edition );
	  MIL << "Locking capability " << capability << endl;
          forEachMatchIn( pool, Dep::PROVIDES, capability, functor::functorRef<bool, const CapAndItem &>(lockItemFunc) );
        }
        catch ( const Exception &e )
        {
          ERR << "Invalid lock: " << e.msg() << std::endl;
        }
        ++count;

      }
    }

    sqlite3_finalize( handle );

    if (rc != SQLITE_DONE)
    {
      ERR << "Error reading lock packages: " << sqlite3_errmsg (db) << endl;
      return -1;
    }

    return count;
}

