/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbProductImpl.h
 *
*/

#include "DbProductImpl.h"
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
//        CLASS NAME : DbProductImpl
//
///////////////////////////////////////////////////////////////////

/** Default ctor
*/
DbProductImpl::DbProductImpl (Source_Ref source_r)
    : _source (source_r)
    , _zmdid(0)
{}

/**
 * read package specific data from handle
 * throw() on error
 */

void
DbProductImpl::readHandle( sqlite_int64 id, sqlite3_stmt *handle )
{
  _zmdid = id;

  //      0   1     2        3        4      5
  //     id, name, version, release, epoch, arch
  //      6               7
  //      installed_size, catalog,"
  //      8          9      10      11        12, 13, 14, 15
  //   installed, local, status, category, distribution_name, distribution_version, distribution_release, distribution_epoch "
  
  const char * text = ((const char *) sqlite3_column_text( handle, 7 ));
  if (text != NULL)
    _category = text;

  const char *dist_name = reinterpret_cast<const char *>(sqlite3_column_text( handle, 12 ));
  const char *dist_ver = reinterpret_cast<const char *>(sqlite3_column_text( handle, 13 ));
  const char *dist_rel = reinterpret_cast<const char *>(sqlite3_column_text( handle, 14 ));
  int dist_epoch = sqlite3_column_int( handle, 15 );
  
#define CHECKED_STRING(X)  ( (X == NULL) ? "" : X )
  
  _distribution_name = CHECKED_STRING(dist_name);
  _distribution_edition = Edition( CHECKED_STRING(dist_ver), CHECKED_STRING(dist_rel), dist_epoch);
  
  return;
}


Source_Ref
DbProductImpl::source() const
{
  return _source;
}

ZmdId DbProductImpl::zmdid() const
{
  return _zmdid;
}

/** Product summary */
TranslatedText DbProductImpl::summary() const
{
  return _summary;
}

/** Product description */
TranslatedText DbProductImpl::description() const
{
  return _description;
}

/** Get the category of the product - addon or base*/
std::string DbProductImpl::category() const
{
  return _category;
}

/** Get the vendor of the product */
Label DbProductImpl::vendor() const
{
  return _vendor;
}

/** Get the name of the product to be presented to user */
Label DbProductImpl::displayName( const Locale & locale_r ) const
{
  return _displayName;
}

/** */
Url DbProductImpl::releaseNotesUrl() const
{
  return _releaseNotesUrl;
}

std::string DbProductImpl::distributionName() const
{
  return _distribution_name;
}

Edition DbProductImpl::distributionEdition() const
{
  return _distribution_edition;
}

/////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
