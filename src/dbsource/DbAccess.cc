/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbAccess.cc
 *
*/

#include <iostream>
#include <fstream>

#include "zypp/base/Logger.h"
#include "zypp/ZYppFactory.h"
#include "zypp/Source.h"
#include "zypp/detail/ImplConnect.h"
#include "zypp/source/PackageDelta.h"
#include "DbAccess.h"

IMPL_PTR_TYPE(DbAccess);

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "DbAccess"

using namespace std;
using namespace zypp;
using namespace zypp::packagedelta;

static struct archrc
{
  char *arch;
  RCArch rc;
}
archtable[] = {
                { "noarch",	RC_ARCH_NOARCH },
                { "i386",	RC_ARCH_I386 },
                { "i486",	RC_ARCH_I486 },
                { "i586",	RC_ARCH_I586 },
                { "i686",	RC_ARCH_I686 },
                { "x86_64",	RC_ARCH_X86_64 },
                { "ia32e",	RC_ARCH_IA32E },
                { "athlon",	RC_ARCH_ATHLON },
                { "ppc",	RC_ARCH_PPC },
                { "ppc64",	RC_ARCH_PPC64 },
                { "s390",	RC_ARCH_S390 },
                { "s390x",	RC_ARCH_S390X },
                { "ia64",	RC_ARCH_IA64 },
                { "sparc",	RC_ARCH_SPARC },
                { "sparc64",	RC_ARCH_SPARC64 },
                { NULL,		RC_ARCH_UNKNOWN }
              };


//----------------------------------------------------------------------------

// check if source is local for zypp

static bool
source_is_local( Source_Ref source )
{
  return false;

  if (!source) return true;

  string scheme = source.url().getScheme();
//    DBG << "Source url '" << src.url() << "', scheme '" << scheme << "'" << endl;
  return (scheme == "cd"
          || scheme == "dvd"
          || scheme == "file"
          || scheme == "dir"
          || scheme == "hd"
          || scheme == "iso"
          || scheme == "smb"
          || scheme == "nfs");
}

//----------------------------------------------------------------------------

// Convert ZYPP relation operator to ZMD RCResolvableRelation

RCResolvableRelation
DbAccess::Rel2Rc (Rel op)
{

  /* This enum is here so that gdb can give us pretty strings */

  switch (op.inSwitch())
  {
  case Rel::EQ_e:
    return RC_RELATION_EQUAL;
    break;
  case Rel::NE_e:
    return RC_RELATION_NOT_EQUAL;
    break;
  case Rel::LT_e:
    return RC_RELATION_LESS;
    break;
  case Rel::LE_e:
    return RC_RELATION_LESS_EQUAL;
    break;
  case Rel::GT_e:
    return RC_RELATION_GREATER;
    break;
  case Rel::GE_e:
    return RC_RELATION_GREATER_EQUAL;
    break;
  case Rel::ANY_e:
    return RC_RELATION_ANY;
    break;
  case Rel::NONE_e:
    return RC_RELATION_NONE;
    break;
  }
  return RC_RELATION_INVALID;
}


Rel
DbAccess::Rc2Rel (RCResolvableRelation rel)
{
  switch (rel)
  {
  case RC_RELATION_INVALID:
    return Rel::NONE;
    break;
  case RC_RELATION_ANY:
    return Rel::ANY;
    break;
  case RC_RELATION_EQUAL:
    return Rel::EQ;
    break;
  case RC_RELATION_LESS:
    return Rel::LT;
    break;
  case RC_RELATION_LESS_EQUAL:
    return Rel::LE;
    break;
  case RC_RELATION_GREATER:
    return Rel::GT;
    break;
  case RC_RELATION_GREATER_EQUAL:
    return Rel::GE;
    break;
  case RC_RELATION_NOT_EQUAL:
    return Rel::NE;
    break;
  case RC_RELATION_NONE:
    return Rel::ANY;
    break;
  }
  return Rel::NONE;
}

//----------------------------------------------------------------------------

// convert ZYPP architecture string to ZMD int

RCArch
DbAccess::Arch2Rc (const Arch & arch)
{
  string arch_str = arch.asString();
  struct archrc *aptr = archtable;
  while (aptr->arch != NULL)
  {
    if (arch_str == aptr->arch)
      break;
    aptr++;
  }

  return aptr->rc;
}

Arch
DbAccess::Rc2Arch (RCArch rc)
{
  if (rc == RC_ARCH_UNKNOWN)
    return Arch();

  struct archrc *aptr = archtable;
  while (aptr->arch != NULL)
  {
    if (aptr->rc == rc)
    {
      return Arch (aptr->arch);
    }
    aptr++;
  }
  WAR << "DbAccess::Rc2Arch(" << rc << ") unknown" << endl;
  return Arch ();
}


// remove Authors from description Text
static string
desc2str (const Text t)
{
  static string s;		// static so we can use sqlite STATIC below
  s.clear();
  string::size_type authors = t.find ("Authors:");		// strip off 'Authors:'

  if (authors == string::npos)
  {	// if no "Authors", point to end of string
    authors = t.size();
  }

  // now remove trailing whitespace

  do
  {
    --authors;
  }
  while (t[authors] == ' ' || t[authors] == '\n');
  s = string( t, 0, authors+1 );

  return s;
}

//----------------------------------------------------------------------------
// convert ZYPP Resolvable kind to ZMD RCDependencyTarget

static RCDependencyTarget
kind2target( Resolvable::Kind kind )
{
  if (kind == ResTraits<Package>::kind)	 return RC_DEP_TARGET_PACKAGE;
  else if (kind == ResTraits<Script>::kind)	 return RC_DEP_TARGET_SCRIPT;
  else if (kind == ResTraits<Message>::kind)	 return RC_DEP_TARGET_MESSAGE;
  else if (kind == ResTraits<Patch>::kind)	 return RC_DEP_TARGET_PATCH;
  else if (kind == ResTraits<Selection>::kind) return RC_DEP_TARGET_SELECTION;
  else if (kind == ResTraits<Pattern>::kind)	 return RC_DEP_TARGET_PATTERN;
  else if (kind == ResTraits<Product>::kind)	 return RC_DEP_TARGET_PRODUCT;
  else if (kind == ResTraits<Language>::kind)	 return RC_DEP_TARGET_LANGUAGE;
  else if (kind == ResTraits<Atom>::kind)	 return RC_DEP_TARGET_ATOM;
  else if (kind == ResTraits<SrcPackage>::kind) return RC_DEP_TARGET_SRC;
  else if (kind == ResTraits<SystemResObject>::kind) return RC_DEP_TARGET_SYSTEM;

  WAR << "Unknown resolvable kind " << kind << endl;
  return RC_DEP_TARGET_UNKNOWN;
}

//----------------------------------------------------------------------------
// convert ZYPP ResStatus to ZMD RCResolvableStatus

static RCResolvableStatus
resstatus2rcstatus( ResStatus status )
{
  if (status.isEstablishedUneeded()) return RC_RES_STATUS_UNNEEDED;
  else if (status.isEstablishedSatisfied()) return RC_RES_STATUS_SATISFIED;
  else if (status.isEstablishedIncomplete()) return RC_RES_STATUS_BROKEN;
  return RC_RES_STATUS_UNDETERMINED;
}

//----------------------------------------------------------------------------

/** Ctor */
DbAccess::DbAccess( const std::string & dbfile_r )
    : _dbfile( dbfile_r )
    , _db( NULL )
    , _insert_res_handle( NULL )
    , _insert_pkg_handle( NULL )
    , _insert_patch_package_handle(0L)
    , _insert_patch_package_baseversion_handle(0L)
    , _insert_delta_package_handle(0L)
    , _insert_message_handle( NULL )
    , _insert_script_handle( NULL )
    , _insert_patch_handle( NULL )
    , _insert_pattern_handle( NULL )
    , _insert_product_handle( NULL )
    , _insert_dep_handle( NULL )
    , _update_catalog_checksum_handle( NULL )
{
  MIL << "DbAccess::DbAccess(" << dbfile_r << ")" << endl;
}


/** Dtor */
DbAccess::~DbAccess( )
{
  closeDb();
}


std::ostream &
DbAccess::dumpOn( std::ostream & str ) const
{
  str << "DbAccess(" << _dbfile << ")" << endl;
  return str;
}


//----------------------------------------------------------------------------

static sqlite3_stmt *
prepare_handle( sqlite3 *db, const string & query )
{
  int rc;
  sqlite3_stmt *handle = NULL;

  rc = sqlite3_prepare (db, query.c_str(), -1, &handle, NULL);

  if (rc != SQLITE_OK)
  {
    ERR << "Can not prepare '" << query << "': " << sqlite3_errmsg (db) << endl;
    handle = NULL;
  }

  return handle;
}


static sqlite3_stmt *
prepare_res_insert (sqlite3 *db)
{
  string query (
    //			  1     2        3        4      5
    "INSERT INTO resolvables (name, version, release, epoch, arch,"
    //			  6               7
    "                         installed_size, catalog,"
    //			  8          9      10      11        12       13
    "                         installed, local, status, category, license, kind) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

  return prepare_handle( db, query );
}


static sqlite3_stmt *
prepare_message_insert (sqlite3 *db)
{
  string query (
    //			      1
    "INSERT INTO message_details (resolvable_id, "
    //			      2
    "                             content) "
    "VALUES (?, ?)"
    "");

  return prepare_handle( db, query );
}


static sqlite3_stmt *
prepare_script_insert (sqlite3 *db)
{
  string query (
    //			      1
    "INSERT INTO script_details (resolvable_id, "
    //			      2          3
    "                             do_script, undo_script) "
    "VALUES (?, ?, ?)"
    "");

  return prepare_handle( db, query );
}


static sqlite3_stmt *
prepare_pkg_insert (sqlite3 *db)
{
  string query (
    //			      1              2          3
    "INSERT INTO package_details (resolvable_id, rpm_group, summary, "
    //			      4            5		6                 7
    "                             description, package_url, package_filename, signature_filename,"
    //			      8	         9             10
    "                             file_size, install_only, media_nr) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    "");

  return prepare_handle( db, query );
}


static sqlite3_stmt *
prepare_patch_insert (sqlite3 *db)
{
  string query (
    //			    1              2
    "INSERT INTO patch_details (resolvable_id, patch_id, "
    //			    3
    "                           creation_time,"
    //			    4       5        6
    "                           reboot, restart, interactive,"
    //			    7        8
    "                           summary, description) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
    "");

  return prepare_handle( db, query );
}


static sqlite3_stmt *
prepare_pattern_insert (sqlite3 *db)
{
  string query (
    //			      1
    "INSERT INTO pattern_details (resolvable_id, "
    //			      2        3
    "                             summary, description) "
    "VALUES (?, ?, ?)"
    "");

  return prepare_handle( db, query );
}


static sqlite3_stmt *
prepare_product_insert (sqlite3 *db)
{
  string query (
    //			      1
    "INSERT INTO product_details (resolvable_id, "
    //			      2        3
    "                             summary, description) "
    "VALUES (?, ?, ?)"
    "");

  return prepare_handle( db, query );
}

static sqlite3_stmt *
prepare_patch_package_insert( sqlite3 *db )
{
  string query(
    "INSERT INTO patch_packages "
    //  1           2         3         4         5              6
    " ( package_id, media_nr, location, checksum, download_size, build_time ) "
    " VALUES (?, ?, ?, ?, ?, ?)");
  return prepare_handle( db, query );
}

static sqlite3_stmt *
prepare_patch_package_baseversion_insert( sqlite3 *db )
{
  string query(
    "INSERT INTO patch_packages_baseversions "
    //  1                   2         3         4
    " ( patch_package_id, version, release, epoch ) "
    " VALUES (?, ?, ?, ?)");
  return prepare_handle( db, query );
}

static sqlite3_stmt *
prepare_delta_package_insert( sqlite3 *db )
{
  string query(
    "INSERT INTO delta_packages "
    //    1           2         3         4         5              6
    " ( package_id, media_nr, location, checksum, download_size, build_time"
    //    7                     8                     9                     10                  11                    12
    "  , baseversion_version, baseversion_release, baseversion_epoch, baseversion_checksum, baseversion_build_time, baseversion_sequence_info ) "
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
  return prepare_handle( db, query );
}

static sqlite3_stmt *
prepare_dep_insert (sqlite3 *db)
{
  string query (
    "INSERT INTO dependencies "
    //  1              2         3     4        5        6      7     8         9
    "  (resolvable_id, dep_type, name, version, release, epoch, arch, relation, dep_target) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

  return prepare_handle( db, query );
}

static sqlite3_stmt *
prepare_catalog_checksum_update(sqlite3 *db)
{
  string query (
    "UPDATE catalogs "
    "SET checksum =?, timestamp =? WHERE id=?");
  return prepare_handle( db, query );
}
    
bool
DbAccess::prepareWrite(void)
{
  XXX << "DbAccess::prepareWrite()" << endl;

  sqlite3_exec (_db, "PRAGMA synchronous = 0", NULL, NULL, NULL);

  bool result = false;

  _insert_res_handle = prepare_res_insert (_db);
  if (_insert_res_handle == NULL)
  {
    goto cleanup;
  }

  _insert_pkg_handle = prepare_pkg_insert (_db);
  if (_insert_pkg_handle == NULL)
  {
    goto cleanup;
  }

  _insert_patch_package_handle = prepare_patch_package_insert (_db);
  if (_insert_patch_package_handle == NULL)
  {
    goto cleanup;
  }

  _insert_patch_package_baseversion_handle = prepare_patch_package_baseversion_insert (_db);
  if (_insert_patch_package_baseversion_handle == NULL)
  {
    goto cleanup;
  }

  _insert_delta_package_handle = prepare_delta_package_insert (_db);
  if (_insert_delta_package_handle == NULL)
  {
    goto cleanup;
  }

  _insert_patch_package_handle = prepare_patch_package_insert (_db);
  if (_insert_patch_package_handle == NULL)
  {
    goto cleanup;
  }
  _insert_message_handle = prepare_message_insert (_db);
  if (_insert_message_handle == NULL)
  {
    goto cleanup;
  }

  _insert_script_handle = prepare_script_insert (_db);
  if (_insert_script_handle == NULL)
  {
    goto cleanup;
  }

  _insert_patch_handle = prepare_patch_insert (_db);
  if (_insert_patch_handle == NULL)
  {
    goto cleanup;
  }

  _insert_pattern_handle = prepare_pattern_insert (_db);
  if (_insert_pattern_handle == NULL)
  {
    goto cleanup;
  }

  _insert_product_handle = prepare_product_insert (_db);
  if (_insert_product_handle == NULL)
  {
    goto cleanup;
  }

  _insert_dep_handle = prepare_dep_insert (_db);
  if (_insert_dep_handle == NULL)
  {
    goto cleanup;
  }

  _update_catalog_checksum_handle = prepare_catalog_checksum_update(_db);
  if ( _update_catalog_checksum_handle == NULL )
  {
    goto cleanup;
  }
  
  result = true;

cleanup:
  if (result == false)
  {
    closeDb();
  }

  return result;
}


//----------------------------------------------------------------------------

bool
DbAccess::openDb( bool for_writing )
{
  XXX << "DbAccess::openDb(" << (for_writing?"write":"read") << ")" << endl;

  if (_db)
  {
    WAR << "Db already open" << endl;
    return true;
  }

  int rc = sqlite3_open (_dbfile.c_str(), &_db);
  if (rc != SQLITE_OK
      || _db == NULL)
  {
    ERR << "Can not open SQL database: " << sqlite3_errmsg (_db) << endl;
    cerr << "1|Can't open " << _dbfile << endl;
    return false;
  }

  if (for_writing)
  {
    if (!prepareWrite())
    {
      cerr << "1|Can't prepare sql access handles" << endl;
      return false;
    }
  }

  sqlite3_exec (_db, "BEGIN", NULL, NULL, NULL);

  return true;
}


static void
close_handle( sqlite3_stmt **handle )
{
  if (*handle)
  {
    sqlite3_finalize (*handle);
    *handle = NULL;
  }
  return;
}


void
DbAccess::closeDb(void)
{
  XXX << "DbAccess::closeDb()" << endl;

  commit();

  close_handle( &_insert_res_handle );
  close_handle( &_insert_pkg_handle );
  close_handle( &_insert_patch_package_handle );
  close_handle( &_insert_patch_package_baseversion_handle );
  close_handle( &_insert_delta_package_handle );
  close_handle( &_insert_message_handle );
  close_handle( &_insert_script_handle );
  close_handle( &_insert_patch_handle );
  close_handle( &_insert_pattern_handle );
  close_handle( &_insert_product_handle );
  close_handle( &_insert_dep_handle );

  if (_db)
  {
    sqlite3_close (_db);
    _db = NULL;
  }
  return;
}


void
DbAccess::commit(void)
{
  if (_db)
    sqlite3_exec (_db, "COMMIT", NULL, NULL, NULL);
}


void
DbAccess::updateCatalogChecksum( const std::string &catalog, const std::string &checksum, const zypp::Date &timestamp )
{
  int rc;
  sqlite3_stmt *handle = _update_catalog_checksum_handle;
  
  rc = sqlite3_bind_text( handle, 1, checksum.c_str(), -1, SQLITE_STATIC );	// checksum
  rc = sqlite3_bind_int( handle, 2, timestamp);						// type (provides, requires, ...)
  rc = sqlite3_bind_text( handle, 3, catalog.c_str(), -1, SQLITE_STATIC );	// id
  
  rc = sqlite3_step( handle);
  sqlite3_reset( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error updating catalog checksum and timestamp: " << sqlite3_errmsg (_db) << endl;
  }
  return;
}

//----------------------------------------------------------------------------
// dependency

void
DbAccess::writeDependency( sqlite_int64 res_id, RCDependencyType type, const zypp::CapSet & capabilities)
{
  XXX << "DbAccess::writeDependency(" << res_id << ", " << type << ", ...)" << endl;

  int rc;
  sqlite3_stmt *handle = _insert_dep_handle;

  if (capabilities.empty())
    return;

  for (zypp::CapSet::const_iterator iter = capabilities.begin(); iter != capabilities.end(); ++iter)
  {
    XXX << "Cap " << *iter << endl;
    RCDependencyTarget refers = kind2target( iter->refers() );
    if (refers == RC_DEP_TARGET_UNKNOWN) continue;

    sqlite3_bind_int64( handle, 1, res_id);						// who issues the dependency
    sqlite3_bind_int( handle, 2, type);						// type (provides, requires, ...)
    sqlite3_bind_text( handle, 3, iter->index().c_str(), -1, SQLITE_STATIC );	// tag

    Edition edition = iter->edition();
    string version = edition.version();
    string release = edition.release();

    if (iter->op() != Rel::NONE							// operator and edition given ?
        && iter->op() != Rel::ANY
        && edition != Edition::noedition)
    {
      sqlite3_bind_text( handle, 4, version.c_str(), -1, SQLITE_STATIC );	// version
      sqlite3_bind_text( handle, 5, release.c_str(), -1, SQLITE_STATIC );	// release
      Edition::epoch_t epoch = edition.epoch();
      if (epoch != Edition::noepoch)
      {
        sqlite3_bind_int( handle, 6, epoch);						// epoch
      }
      else
      {
        sqlite3_bind_int( handle, 6, 0);
      }
      sqlite3_bind_int( handle, 7, -1);							// arch
      sqlite3_bind_int( handle, 8, Rel2Rc( iter->op() ));					// operation (==, <, <=, ...)
    }
    else
    {
      // no operation or edition given
      sqlite3_bind_text( handle, 4, NULL, -1, SQLITE_STATIC );
      sqlite3_bind_text( handle, 5, NULL, -1, SQLITE_STATIC );
      sqlite3_bind_int( handle, 6, 0);
      sqlite3_bind_int( handle, 7, -1);				// arch
      sqlite3_bind_int( handle, 8, RC_RELATION_NONE );
    }

    sqlite3_bind_int( handle, 9, refers );					// resolvable kind the dependency refers to

    rc = sqlite3_step( handle);
    sqlite3_reset( handle);

    if (rc != SQLITE_DONE)
    {
      ERR << "Error adding package to SQL: " << sqlite3_errmsg (_db) << endl;
    }
  }
  return;
}


void
DbAccess::writeDependencies(sqlite_int64 id, Resolvable::constPtr res)
{
  XXX << "DbAccess::writeDependencies(" << id << ", " << *res << ")" << endl;

  writeDependency( id, RC_DEP_TYPE_REQUIRE,	 res->dep (Dep::REQUIRES));
  writeDependency( id, RC_DEP_TYPE_PROVIDE,	 res->dep (Dep::PROVIDES));
  writeDependency( id, RC_DEP_TYPE_CONFLICT,	 res->dep (Dep::CONFLICTS));
  writeDependency( id, RC_DEP_TYPE_OBSOLETE,	 res->dep (Dep::OBSOLETES));
  writeDependency( id, RC_DEP_TYPE_PREREQUIRE, res->dep (Dep::PREREQUIRES));
  writeDependency( id, RC_DEP_TYPE_FRESHEN,	 res->dep (Dep::FRESHENS));
  writeDependency( id, RC_DEP_TYPE_RECOMMEND,  res->dep (Dep::RECOMMENDS));
  writeDependency( id, RC_DEP_TYPE_SUGGEST,	 res->dep (Dep::SUGGESTS));
  writeDependency( id, RC_DEP_TYPE_SUPPLEMENT, res->dep (Dep::SUPPLEMENTS));
  writeDependency( id, RC_DEP_TYPE_ENHANCE,	 res->dep (Dep::ENHANCES));
}


//----------------------------------------------------------------------------
// package

sqlite_int64
DbAccess::writePackage( sqlite_int64 id, Package::constPtr pkg, Ownership owner )
{
  XXX <<  "DbAccess::writePackage(" << id << ", " << *pkg << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_pkg_handle;

  sqlite3_bind_int64( handle, 1, id);
  sqlite3_bind_text( handle, 2, pkg->group().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 3, pkg->summary().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 4, desc2str(pkg->description()).c_str(), -1, SQLITE_STATIC );

  const char *package_url = pkg->location().asString().c_str();
  if (package_url[0] == '.' && package_url[1] == '/') package_url += 2;		// strip leading "./"

  const char *url = package_url;
  const char *filename = NULL;

  switch ( owner )
  {
  case ZYPP_OWNED:
    filename = "";
    break;
  case ZMD_OWNED:
    break;
  case LOCAL_FILE:
    url = NULL;
    filename = package_url;
    break;
  default:
    ERR << "Unknown ownership" << endl;
    break;
  }

  sqlite3_bind_text( handle, 5, url, -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 6, filename, -1, SQLITE_STATIC );

  sqlite3_bind_text( handle, 7, NULL, -1, SQLITE_STATIC );			// signature_filename
  sqlite3_bind_int( handle, 8, pkg->size() );
  sqlite3_bind_int( handle, 9, pkg->installOnly() ? 1 : 0 );
  sqlite3_bind_int( handle, 10, pkg->sourceMediaNr() );

  rc = sqlite3_step( handle);
  
  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding package to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 package_rowid = sqlite3_last_insert_rowid (_db);
  sqlite3_reset( handle);


  // access package implementation to get delta and patch rpm info
  detail::ResImplTraits<Package::Impl>::constPtr pipp( detail::ImplConnect::resimpl( pkg ) );

  // save delta rpms
  std::list<DeltaRpm> deltas = pipp->deltaRpms();
  if ( ! deltas.empty() )
  {
    //DBG << "Package " << pkg->name() << " " << pkg->edition() << " contains " << deltas.size() << " delta rpms" << std::endl;
    for ( std::list<DeltaRpm>::const_iterator it = deltas.begin(); it != deltas.end(); ++it )
    {
      writeDeltaPackage ( package_rowid, *it );
    }
  }
  
  // save patch rpms
  std::list<PatchRpm> patches = pipp->patchRpms();
  if ( ! patches.empty() )
  {
    //DBG << "Package " << pkg->name() << " " << pkg->edition() << " contains " << patches.size() << " patch rpms" << std::endl;
    for ( std::list<PatchRpm>::const_iterator it = patches.begin(); it != patches.end(); ++it )
    {
      writePatchPackage ( package_rowid, *it );
    }
  }
  return package_rowid;
}

// patch rpm
//----------------------------------------------------------------------------
sqlite_int64
DbAccess::writePatchPackage (sqlite_int64 package_id, const PatchRpm &patch_pkg )
{
  /*  " ( package_id, media_nr, location, checksum, download_size, build_time ) "
        " VALUES (?, ?, ?, ?, ?, ?)");*/
  //XXX << package_id << ", " << patch_pkg << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_patch_package_handle;

  sqlite3_bind_int64( handle, 1, package_id);

  // on media location data
  sqlite3_bind_int( handle, 2, patch_pkg.location().medianr() );
  sqlite3_bind_text( handle, 3, patch_pkg.location().filename().asString().c_str(), -1, SQLITE_STATIC );
  std::string checksum_encoded_string = patch_pkg.location().checksum().type() + ":" + patch_pkg.location().checksum().checksum();
  sqlite3_bind_text( handle, 4, checksum_encoded_string.c_str() , -1, SQLITE_STATIC );
  sqlite3_bind_int( handle, 5, patch_pkg.location().downloadsize() );

  // buildtime data
  sqlite3_bind_int( handle, 6, patch_pkg.buildtime() );

  rc = sqlite3_step( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding patch package to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);

  // base version data
  for ( PatchRpm::BaseVersions::const_iterator it = patch_pkg.baseversions().begin(); it != patch_pkg.baseversions().end(); ++it)
  {
    writePatchPackageBaseversion( rowid, *it );
  }

  sqlite3_reset( handle);
  return rowid;
}

sqlite_int64
DbAccess::writePatchPackageBaseversion(sqlite_int64 patch_package_id, const PatchRpm::BaseVersion &baseversion )
{
  //MIL << "writing " << baseversion << std::endl;
  int rc;
  sqlite3_stmt *handle = _insert_patch_package_baseversion_handle;

  sqlite3_bind_int64( handle, 1, patch_package_id);

  sqlite3_bind_text( handle, 2, baseversion.version().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 3, baseversion.release().c_str(), -1, SQLITE_STATIC );
  if ( baseversion.epoch() == Edition::noepoch)
  {
    sqlite3_bind_int( handle, 4, 0);
  }
  else
  {
    sqlite3_bind_int( handle, 4,  baseversion.epoch() );
  }

  rc = sqlite3_step( handle);
  
  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding patch package baseversion to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);
  
  sqlite3_reset( handle);

  return rowid;
}


// delta rpm
//----------------------------------------------------------------------------
sqlite_int64
DbAccess::writeDeltaPackage (sqlite_int64 package_id, const DeltaRpm &delta_pkg )
{
  //XXX << package_id << ", " << delta_pkg << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_delta_package_handle;

  //  "INSERT INTO delta_packages "
    //    1           2         3         4         5              6
  //  " ( package_id, media_nr, location, checksum, download_size, build_time"
    //    7                     8                     9                     10                  11                    12
  //  "  , baseversion_version, baseversion_release, baseversion_epoch, baseversion_checksum, baseversion_build_time, baseversion_sequence_info ) "
  //  " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
  
  MIL << "Delta: " << std::endl;
  MIL << package_id << " " << delta_pkg.location().medianr() << " " << delta_pkg.location().filename().asString() << std::endl;
  
  try {
    rc = sqlite3_bind_int64( handle, 1, package_id);
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    // on media location data
    rc = sqlite3_bind_int( handle, 2, delta_pkg.location().medianr() );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    rc = sqlite3_bind_text( handle, 3, delta_pkg.location().filename().asString().c_str(), -1, SQLITE_STATIC );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    std::string checksum_encoded_string = delta_pkg.location().checksum().type() + ":" + delta_pkg.location().checksum().checksum();
    
    // this segfaults
    rc = sqlite3_bind_text( handle, 4, checksum_encoded_string.c_str() , -1, SQLITE_STATIC );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    rc = sqlite3_bind_int( handle, 5, (int)  delta_pkg.location().downloadsize() );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    // delta rpm own data
    rc = sqlite3_bind_int( handle, 6, (int) delta_pkg.buildtime() );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
  
    // base version data
    rc = sqlite3_bind_text( handle, 7, delta_pkg.baseversion().edition().version().c_str(), -1, SQLITE_STATIC );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    rc = sqlite3_bind_text( handle, 8, delta_pkg.baseversion().edition().release().c_str(), -1, SQLITE_STATIC );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    if (delta_pkg.baseversion().edition().epoch() == Edition::noepoch)
    {
      rc = sqlite3_bind_int( handle, 9, 0);
      if ( rc != SQLITE_OK )
        ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    }
    else
    {
      rc = sqlite3_bind_int( handle, 9, (int)  delta_pkg.baseversion().edition().epoch() );
      if ( rc != SQLITE_OK )
        ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    }
    checksum_encoded_string = delta_pkg.baseversion().checksum().type() + ":" + delta_pkg.baseversion().checksum().checksum();
    rc = sqlite3_bind_text( handle, 10, checksum_encoded_string.c_str() , -1, SQLITE_STATIC );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    rc = sqlite3_bind_int( handle, 11, (int)  delta_pkg.baseversion().buildtime() );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));
    
    rc = sqlite3_bind_text( handle, 12, delta_pkg.baseversion().sequenceinfo().c_str() , -1, SQLITE_STATIC );
    if ( rc != SQLITE_OK )
      ZYPP_THROW(Exception(sqlite3_errmsg(_db)));

  }
  catch ( const Exception &e )
  {
    ERR << "Error adding delta package to SQL: " << e.msg() << endl;
    return -1;
  }
   
  
  //FIXME add build time data


  rc = sqlite3_step( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding delta package to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);
  sqlite3_reset( handle);
  return rowid;
}


//----------------------------------------------------------------------------
// message

sqlite_int64
DbAccess::writeMessage (sqlite_int64 id, Message::constPtr message )
{
  XXX <<  "DbAccess::writeMessage(" << id << ", " << *message << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_message_handle;

  sqlite3_bind_int64( handle, 1, id);
  sqlite3_bind_text( handle, 2, message->text().asString().c_str(), -1, SQLITE_STATIC );

  rc = sqlite3_step( handle);
  sqlite3_reset( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding message to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);

  return rowid;
}


//----------------------------------------------------------------------------
// script

static string
readFromPath( const Pathname & p )
{
  string result;
  string line;
  try
  {
    ifstream infile( p.asString().c_str() );
    while (getline( infile, line ))
    {
      result += line;
      result += "\n";
    }
  }
  catch ( const Exception & excpt_r )
  {
    ZYPP_CAUGHT( excpt_r );
  }
  return result;
}

sqlite_int64
DbAccess::writeScript (sqlite_int64 id, Script::constPtr script)
{
  XXX <<  "DbAccess::writeScript(" << id << ", " << *script << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_script_handle;

  sqlite3_bind_int64( handle, 1, id);
  string do_script( readFromPath( script->do_script() ) );
  string undo_script( readFromPath( script->undo_script() ) );
  sqlite3_bind_text( handle, 2, do_script.c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 3, undo_script.c_str(), -1, SQLITE_STATIC );

  rc = sqlite3_step( handle);
  sqlite3_reset( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding message to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);

  return rowid;
}


//----------------------------------------------------------------------------
// patch

sqlite_int64
DbAccess::writePatch (sqlite_int64 id, Patch::constPtr patch )
{
  XXX <<  "DbAccess::writePatch(" << id << ", " << *patch << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_patch_handle;

  sqlite3_bind_int64( handle, 1, id );
  sqlite3_bind_text( handle, 2, patch->id().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_int64( handle, 3, patch->timestamp() );
  sqlite3_bind_int( handle, 4, patch->reboot_needed() ? 1 : 0 );
  sqlite3_bind_int( handle, 5, patch->affects_pkg_manager() ? 1 : 0 );
  sqlite3_bind_int( handle, 6, patch->interactive() ? 1 : 0 );
  sqlite3_bind_text( handle, 7, patch->summary().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 8, desc2str(patch->description()).c_str(), -1, SQLITE_STATIC );

  rc = sqlite3_step( handle);
  sqlite3_reset( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding patch to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);

  return rowid;
}


//----------------------------------------------------------------------------
// pattern

sqlite_int64
DbAccess::writePattern (sqlite_int64 id, Pattern::constPtr pattern )
{
  XXX <<  "DbAccess::writePattern(" << id << ", " << *pattern << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_pattern_handle;

  sqlite3_bind_int64( handle, 1, id);
  sqlite3_bind_text( handle, 2, pattern->summary().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 3, desc2str(pattern->description()).c_str(), -1, SQLITE_STATIC );

  rc = sqlite3_step( handle);
  sqlite3_reset( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding pattern to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);

  return rowid;
}


//----------------------------------------------------------------------------
// product

sqlite_int64
DbAccess::writeProduct (sqlite_int64 id, Product::constPtr product )
{
  XXX <<  "DbAccess::writeProduct(" << id << ", " << *product << ")" << endl;
  int rc;
  sqlite3_stmt *handle = _insert_product_handle;

  sqlite3_bind_int64( handle, 1, id);
  sqlite3_bind_text( handle, 2, product->summary().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 3, desc2str(product->description()).c_str(), -1, SQLITE_STATIC );

  rc = sqlite3_step( handle);
  sqlite3_reset( handle);

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding product to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);

  return rowid;
}


//----------------------------------------------------------------------------
// resolvable

// write ResObject to resolvables table
//  return rowid (> 0) on success
//  return < 0 on failure
//  return == 0 if this kind of resolvable is to be skipped

sqlite_int64
DbAccess::writeResObject( ResObject::constPtr obj, ResStatus status, const char *catalog, Ownership owner )
{
  XXX << "DbAccess::writeResObject (" << *obj << ", " << status << ")" << endl;

  if (obj->kind() == ResTraits<SystemResObject>::kind)
    return 0;

  Resolvable::constPtr res = obj;
  Package::constPtr pkg = asKind<Package>(res);
  Message::constPtr message = asKind<Message>(res);
  Script::constPtr script = asKind<Script>(res);
  Patch::constPtr patch = asKind<Patch>(res);
  Pattern::constPtr pattern = asKind<Pattern>(res);
  Product::constPtr product = asKind<Product>(res);

  zypp::License license;

  int rc;
  sqlite3_stmt *handle = _insert_res_handle;

  // write NVRAD and all the rest

  sqlite3_bind_text( handle, 1, obj->name().c_str(), -1, SQLITE_STATIC );
  Edition ed = obj->edition();
  sqlite3_bind_text( handle, 2, ed.version().c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 3, ed.release().c_str(), -1, SQLITE_STATIC );
  if (ed.epoch() == Edition::noepoch)
  {
    sqlite3_bind_int( handle, 4, 0);
  }
  else
  {
    sqlite3_bind_int( handle, 4, ed.epoch());
  }
  sqlite3_bind_int( handle, 5, Arch2Rc (obj->arch()) );

  sqlite3_bind_int64( handle, 6, obj->size() );
  if (catalog != NULL)
    sqlite3_bind_text( handle, 7, catalog, -1, SQLITE_STATIC );
  else
    sqlite3_bind_text( handle, 7, obj->source().alias().c_str(), -1, SQLITE_STATIC );

  sqlite3_bind_int( handle,  8, status.isInstalled() ? 1 : 0);

  Source_Ref src( obj->source() );
  if (source_is_local( src ))
  {
    sqlite3_bind_int( handle, 9, 1 );					//local_package
  }
  else
  {
    sqlite3_bind_int( handle, 9, 0 );
  }
  sqlite3_bind_int( handle, 10, resstatus2rcstatus( status ) );

  if (pkg != NULL)
  {
    license = pkg->licenseToConfirm();
    if (license.empty())
    {
      sqlite3_bind_text( handle, 12, NULL, -1, SQLITE_STATIC );
    }
    else
    {
      sqlite3_bind_text( handle, 12, license.c_str(), -1, SQLITE_STATIC );
    }
  }
  else if (patch != NULL)
  {
    sqlite3_bind_text( handle, 11, patch->category().c_str(), -1, SQLITE_STATIC );
  }
  else if (product != NULL)
  {
    sqlite3_bind_text( handle, 11, product->category().c_str(), -1, SQLITE_STATIC );
    license = product->licenseToConfirm();
    if (license.empty())
    {
      sqlite3_bind_text( handle, 12, NULL, -1, SQLITE_STATIC );
    }
    else
    {
      sqlite3_bind_text( handle, 12, license.c_str(), -1, SQLITE_STATIC );
    }
  }

  sqlite3_bind_int( handle, 13, kind2target( obj->kind() ) );

  rc = sqlite3_step( handle );
  sqlite3_reset( handle );

  if (rc != SQLITE_DONE)
  {
    ERR << "Error adding package to SQL: " << sqlite3_errmsg (_db) << endl;
    return -1;
  }
  sqlite_int64 rowid = sqlite3_last_insert_rowid (_db);

  // now write the respective _details table

  if (pkg != NULL) writePackage( rowid, pkg, owner );
  else if (message != NULL) writeMessage( rowid, message );
  else if (script != NULL) writeScript( rowid, script );
  else if (patch != NULL) writePatch( rowid, patch );
  else if (pattern != NULL) writePattern( rowid, pattern );
  else if (product != NULL) writeProduct( rowid, product );

  writeDependencies (rowid, obj);

  return rowid;
}


//----------------------------------------------------------------------------
/** check if catalog exists */
bool
DbAccess::haveCatalog( const std::string & catalog )
{
  string query ("SELECT * FROM catalogs WHERE id = ? ");

  sqlite3_stmt *handle = prepare_handle( _db, query );
  if (handle == NULL)
  {
    return false;
  }

  sqlite3_bind_text( handle, 1, catalog.c_str(), -1, SQLITE_STATIC );

  int rc = sqlite3_step( handle);
  if (rc == SQLITE_ROW)
  {
    DBG << "Found catalog" << endl;
  }
  else if (rc != SQLITE_DONE)
  {
    ERR << "rc " << rc << ": " << sqlite3_errmsg (_db) << endl;
  }
  sqlite3_reset( handle);

  return (rc == SQLITE_ROW);
}

DBCatalogEntry DbAccess::getCatalogEntry( const std::string &catalog )
{
  // FIXME
  return DBCatalogEntry();
}

/** insert catalog */
bool
DbAccess::insertCatalog( const DBCatalogEntry &entry  )
{
  string query ("INSERT INTO catalogs(id,name,alias,description) VALUES (?,?,?,?) ");

  sqlite3_stmt *handle = prepare_handle( _db, query );
  if (handle == NULL)
  {
    return false;
  }

  sqlite3_bind_text( handle, 1, entry.catalog.c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 2, entry.name.c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 3, entry.alias.c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( handle, 4, entry.description.c_str(), -1, SQLITE_STATIC );

  int rc = sqlite3_step( handle);
  if (rc != SQLITE_DONE)
  {
    ERR << "rc " << rc << "Error writing catalog: " << sqlite3_errmsg (_db) << endl;
  }
  sqlite3_reset( handle);

  return (rc == SQLITE_DONE);
}


/** update catalog */
bool
DbAccess::updateCatalog( const DBCatalogEntry &entry )
{

  DBG << "updateCatalog(" << entry.catalog << ", " << entry.name << ", " << entry.alias << ", " << entry.description << ")" << endl;

  //                    0    1     2
  string query ("SELECT name,alias,description FROM catalogs WHERE id = ? ");

  sqlite3_stmt *sel_handle = prepare_handle( _db, query );
  if (sel_handle == NULL)
  {
    ERR << "Can't prepare SELECT query: " << sqlite3_errmsg (_db) << endl;
    return false;
  }

  sqlite3_bind_text( sel_handle, 1, entry.catalog.c_str(), -1, SQLITE_STATIC );

  int rc = sqlite3_step( sel_handle);
  if (rc == SQLITE_ROW)
  {
    DBG << "Found catalog" << endl;
  }
  else if (rc != SQLITE_DONE)
  {
    ERR << "rc " << rc << ": " << sqlite3_errmsg (_db) << endl;
  }

  if (rc != SQLITE_ROW)
  {
    sqlite3_reset( sel_handle );
    return false;
  }

  string c_name, c_alias, c_description;

  // now set the c_* string to either the original or the updated (if non-empty) values

  const char *text;
  if (entry.name.empty())
  {
    text = (const char *) sqlite3_column_text( sel_handle, 0 );
    if (text != NULL)
      c_name = text;
  }
  else
  {
    c_name = entry.name;
  }

  if (entry.alias.empty())
  {
    text = (const char *) sqlite3_column_text( sel_handle, 1 );
    if (text != NULL)
      c_alias = text;
  }
  else
  {
    c_alias = entry.alias;
  }

  if (entry.description.empty())
  {
    text = (const char *) sqlite3_column_text( sel_handle, 2 );
    if (text != NULL)
      c_description = text;
  }
  else
  {
    c_description = entry.description;
  }

  //                                  1          2                3            4
  query = "UPDATE catalogs SET name = ?, alias = ?, description = ? WHERE id = ?";
  sqlite3_stmt *upd_handle = prepare_handle( _db, query );
  if (upd_handle == NULL)
  {
    ERR << "Can't prepare UPDATE query: " << sqlite3_errmsg (_db) << endl;
    return false;
  }

  sqlite3_bind_text( upd_handle, 1, c_name.c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( upd_handle, 2, c_alias.c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( upd_handle, 3, c_description.c_str(), -1, SQLITE_STATIC );
  sqlite3_bind_text( upd_handle, 4, entry.catalog.c_str(), -1, SQLITE_STATIC );

  rc = sqlite3_step( upd_handle );
  if (rc != SQLITE_DONE)
  {
    ERR << "rc " << rc << "Error writing catalog: " << sqlite3_errmsg (_db) << endl;
  }

  sqlite3_reset( sel_handle );
  sqlite3_reset( upd_handle );

  return (rc == SQLITE_DONE);
}


/** remove catalog, remove all resolvables of this catalog */
bool
DbAccess::removeCatalog( const std::string & catalog )
{
  string query ("DELETE FROM catalogs where id = ? ");

  sqlite3_stmt *handle = prepare_handle( _db, query );
  if (handle == NULL)
  {
    return false;
  }

  sqlite3_bind_text( handle, 1, catalog.c_str(), -1, SQLITE_STATIC );

  int rc = sqlite3_step( handle);
  if (rc != SQLITE_DONE)
  {
    ERR << "rc " << rc << ", Error removing catalog: " << sqlite3_errmsg (_db) << endl;
  }
  sqlite3_reset( handle);

  return (rc == SQLITE_DONE);
}


/** empty catalog, remove all resolvables belonging to this catalog */
/* parameter 'const char ** because of callers */
bool
DbAccess::emptyCatalog( const std::string &catalog )
{
  string query ("DELETE FROM resolvables where catalog = ? ");

  sqlite3_stmt *handle = prepare_handle( _db, query );
  if (handle == NULL)
  {
    return false;
  }

  sqlite3_bind_text( handle, 1, catalog.c_str(), -1, SQLITE_STATIC );

  int rc = sqlite3_step( handle);
  if (rc != SQLITE_DONE)
  {
    ERR << "rc " << rc << ", Error emptying catalog: " << sqlite3_errmsg (_db) << endl;
  }
  sqlite3_reset( handle);

  return (rc == SQLITE_DONE);
}

//----------------------------------------------------------------------------
// store

void
DbAccess::writeStore( const zypp::ResStore & store, ResStatus status, const char *catalog, Ownership owner )
{
  XXX << "DbAccess::writeStore()" << endl;

  if (store.empty())
  {
    ERR << "Store is empty." << endl;
    return;
  }

  //emptyCatalog( catalog );

  Arch sysarch = getZYpp()->architecture();

  int count = 0;
  sqlite_int64 rowid = 0;
  for (ResStore::const_iterator iter = store.begin(); iter != store.end(); ++iter)
  {
    ResObject::constPtr obj = *iter;
    if (!obj)
    {
      WAR << "Huh ? No object ?" << endl;
      continue;
    }

    if (obj->kind() != ResTraits<SrcPackage>::kind		// don't write src/nosrc packages
        && ( status == ResStatus::installed			// installed ones are ok
             || obj->kind() == ResTraits<Atom>::kind		//  and atoms because we need them for multi-arch patch requirements
             || obj->arch().compatibleWith( sysarch ) ) )	//   and architecturally compatible ones
    {
      rowid = writeResObject( obj, status, catalog, owner );
      if (rowid < 0)		// rowid < 0 means 'error'
        break;
      if (rowid > 0)		// rowid == 0 means 'skip'
        ++count;
    }
    else
    {
      DBG << "Not writing " << *obj << endl;
    }
  }

  MIL << "Wrote " << count << " resolvables to database, last rowid " << rowid << endl;
  return;
}

//----------------------------------------------------------------------------
// pool

void
DbAccess::writePool( const zypp::ResPool & pool, const char *catalog )
{
  XXX << "DbAccess::writePool()" << endl;

  if (pool.empty())
  {
    ERR << "Pool is empty" << endl;
    return;
  }

  emptyCatalog( catalog );

  int count = 0;
  sqlite_int64 rowid = 0;
  for (ResPool::const_iterator iter = pool.begin(); iter != pool.end(); ++iter)
  {
    rowid = writeResObject( iter->resolvable(), iter->status(), catalog );
    if (rowid < 0)
      break;
    if (rowid > 0)			// rowid == 0 means 'skip'
      ++count;
  }

  MIL << "Wrote " << count << " resolvables to database" << endl;
  return;
}


