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
    item.status().setLock( true, ResStatus::USER);
    MIL << "Locking " << cai_r.item << "(matched by " << _lock_str << ")" << endl;
    return true;
  }

  string _lock_str;
};

// parses an edition from a valid handle
Edition edition_from_handle( sqlite3_stmt *handle, int vercol, int relcol, int epochcol )
{
    const char *ver = 0;
    const char *rel = 0;
    int epoch = 0;
    
    ver = reinterpret_cast<const char*>(sqlite3_column_text( handle, vercol));
    rel = reinterpret_cast<const char*>(sqlite3_column_text( handle, relcol));
    epoch = sqlite3_column_int( handle, epochcol);

    if ( ver )
    {
        if ( rel )
        {
            if ( epoch )
                return Edition(ver, rel, epoch);
            else
                return Edition(ver, rel);
        }
        else
        {
            return Edition(ver);
        }        
    }
    
    return Edition::noedition;
}

/**
 * read 'locks' table, evaluate 'glob' column, assign locks to pool
 *  
 * There are 3 ways to add locks in zmd.
 * 
 *  - Just by package name
 *  - package name with regular expression
 *  - package name with version-release
 *  e.g I have added 3 locks like this
 *  # rug la zmd
 *  # rug la 'zmd*'
 *  # rug la 'zmd' '=' '7.3.0.0-15.2'
 *
 *  # rug ll
 *  # | Name               | Catalog | Importance
 *  --+--------------------+---------+-----------
 *  1 | zmd                | (any)   | (any)
 *  2 | zmd*               | (any)   | (any)
 *  3 | zmd = 7.3.0.0-15.2 | (any)   | (any)
 *
 *  Now if you see the locks table in zmd.db, this is how the 3 locks are stored.
 *  sqlite> select name,version,release,relation,glob from locks;
 *
 *  name | version | release | relation | glob
 *       |         |         | 0        | zmd
 *       |         |         | 0        | zmd*
 *  zmd  | 7.3.0.0 |  15.2   | 1        |
 * 
 *  For name and regular expression locks, the entry goes into the glob field of
 *  the table where was for name-relation-version lock, the glob field is empty.
 *  The entries go into name,version,release and relation fields.
 */
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
      const char *buffer = 0;
      Rel rel;
      Edition edition;
      string name;
      Arch arch;
      
      // we dont need this data yet
      buffer = reinterpret_cast<const char*>(sqlite3_column_text( handle, 1));
      if (buffer)
          name = buffer;

      // pass columns numbers
      try {
          edition = edition_from_handle( handle, 2, 3, 4 );
      }
      catch ( const Edition &e )
      {
          // if the edition was invalid, then log and continue
          ERR << "invalid edition" << endl;
          continue;
      }

      rel = DbAccess::Rc2Rel( (RCResolvableRelation) sqlite3_column_int( handle, 6 ) );
      arch = DbAccess::Rc2Arch( (RCArch) sqlite3_column_int( handle, 5 ) );

      buffer = reinterpret_cast<const char*>(sqlite3_column_text( handle, 7));
      string catalog;
      if (buffer)
         catalog = buffer;

      buffer = reinterpret_cast<const char*>(sqlite3_column_text( handle, 8));
      string glob;
      if (buffer)
          glob = buffer;
      
      //int importance = sqlite3_column_int( handle, 9);
      //int  importance_gteq = sqlite3_column_int( handle, 10);

      // zypp does not provide wildcard or regex support in the Capability matching helpers
      // but it still parses the index if it contains wildcards.
      // so we decompose the capability, and keep the op and edition, while, the name
      // is converted to a regexp and matched against all possible names in the pool
      // Then these names are combined with the original edition and relation and we
      // got a new capability for matching wildcard to use with the capability match
      // helpers


      try
      {
        Capability capability;        

        // first scenario is that glob has the capability so we need to parse the
        // glob capability and extract the name and operator from there.
        if ( ! glob.empty() )
        {
            MIL << "Lock for '" << glob << "'" << endl;
            
            // If there is a glob, ZMD does not support relations and edition, however
            // we parse them anyway. It wont hurt, as rug won't accept getting that
            // lock in the db.
            try
            {    
                // We only use this capability to extract the components of the
                // string expression
                capability = cap_factory.parse( ResTraits<zypp::Package>::kind, glob );
                rel = capability.op();
                edition = capability.edition();
                name = capability.name();
            }
            catch ( const Exception &e )
            {
                ERR << "Can't parse capability in: " << glob << " (" << e.msg() << std::endl;
                ZYPP_RETHROW(e);
            }
        }
        else
        {
            // in the non-glob case, we have already read name, rel and edition from the
            // database.

            // if glob is empty, then the name must be there or we can't do
            // anything
            if ( name.empty() )
                ZYPP_THROW(Exception("lock has both glob and name empty!"));
        }
        
      }
      catch ( const Exception &e )
      {
          ERR << "Can't parse lock!!, skipping" << std::endl;
          continue;
      }

      // Operator NONE is not allowed in Capability
      if (rel == Rel::NONE) rel = Rel::ANY;

      // Now that we have the name (which may be a wildcard), the relation
      // and the edition, we need to expand the wildcard. So we
      // convert the wildcard to a regexp and expand it to matching names.
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

      // Now that the wildcard is a regexp, lets collect all items matching the regexp.
      invokeOnEach( pool.begin(), pool.end(), Match(reg), functor::functorRef<bool, const PoolItem &>(nameMatchFunc) );

      MIL << "Found " << nameMatchFunc.matches.size() << " matches." << endl;

      // now we have all the names matching
      // for each name matching try to match a capability
      ItemLockerFunc lockItemFunc( glob );

      // for every name in the set of matched names, combine it with the original relation 
      // and edition and match items in the pool, which is the final match set.
      for ( set<string>::const_iterator it = nameMatchFunc.matches.begin(); it != nameMatchFunc.matches.end(); ++it )
      {
          string matched_name = *it;
          try
          {   
              Capability capability = cap_factory.parse( ResTraits<zypp::Package>::kind, matched_name, rel, edition );
              MIL << "Locking capability " << capability << endl;
              // for each match, lock the item.
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

