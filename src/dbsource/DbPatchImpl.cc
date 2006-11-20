/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbPatchImpl.cc
 *
*/

#include "DbPatchImpl.h"
#include "zypp/source/SourceImpl.h"
#include "zypp/TranslatedText.h"
#include "zypp/base/String.h"
#include "zypp/base/Logger.h"

using namespace std;
using namespace zypp::detail;

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//        CLASS NAME : DbPatchImpl
//
///////////////////////////////////////////////////////////////////

/** Default ctor
*/
DbPatchImpl::DbPatchImpl (Source_Ref source_r)
    : _source (source_r)
    , _reboot_needed( false )
    , _affects_pkg_manager( false )
{}

/**
 * read patch specific data from handle
 * (see DbSourceImpl, create_patch_handle(), the handle is for the patch_details table)
 * throw() on error


	//      6               7
	"       installed_size, catalog,"
	//      8          9      10        11
	"       installed, local, patch_id, status,"
	//      12             13        14
	"       creation_time, category, reboot,"
	//      15       16 (removed)
	"       restart, interactive"
 */

void
DbPatchImpl::readHandle( sqlite_int64 id, sqlite3_stmt *handle )
{
  _zmdid = id;

  // 1-5: nvra, see DbSourceImpl
  _size_installed = sqlite3_column_int( handle, 6 );
  // 7: catalog
  // 8: installed
  // 9: local
  const char * text = ((const char *) sqlite3_column_text( handle, 10 ));
  if (text != NULL)
    _id = text;
  // 11: status (will re recomputed anyways)
  _timestamp = sqlite3_column_int64( handle, 12 );
  text = (const char *) sqlite3_column_text( handle, 13 );
  if (text != NULL)
    _category = text;

  _reboot_needed = (sqlite3_column_int( handle, 14 ) != 0);
  _affects_pkg_manager = (sqlite3_column_int( handle, 15 ) != 0);

  return;
}


Source_Ref
DbPatchImpl::source() const
{
  return _source;
}

ZmdId DbPatchImpl::zmdid() const
{
  return _zmdid;
}

ByteCount DbPatchImpl::size() const
{
  return _size_installed;
}

/** Patch ID */
std::string DbPatchImpl::id() const
{
  return _id;
}
/** Patch time stamp */
Date DbPatchImpl::timestamp() const
{
  return _timestamp;
}
/** Patch category (recommended, security,...) */
std::string DbPatchImpl::category() const
{
  return _category;
}
/** Does the system need to reboot to finish the update process? */
bool DbPatchImpl::reboot_needed() const
{
  return _reboot_needed;
}
/** Does the patch affect the package manager itself? */
bool DbPatchImpl::affects_pkg_manager() const
{
  return _affects_pkg_manager;
}

/////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
