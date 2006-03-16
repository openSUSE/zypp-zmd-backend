/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbPatternImpl.h
 *
*/

#include "DbPatternImpl.h"
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
//        CLASS NAME : DbPatternImpl
//
///////////////////////////////////////////////////////////////////

/** Default ctor
*/
DbPatternImpl::DbPatternImpl (Source_Ref source_r)
    : _source (source_r)
    , _zmdid(0)
    , _default(false)
    , _visible(false)
{
}

/**
 * read package specific data from handle
 * throw() on error
 */

void
DbPatternImpl::readHandle( sqlite_int64 id, sqlite3_stmt *handle )
{
    _zmdid = id;

    // 1-5: nvra, see DbSourceImpl

    // 6: status (don't care, its recomputed anyways)

    return;
}


Source_Ref
DbPatternImpl::source() const
{ return _source; }

/** Pattern summary */
TranslatedText DbPatternImpl::summary() const
{ return _summary; }

/** Pattern description */
TranslatedText DbPatternImpl::description() const
{ return _description; }

bool DbPatternImpl::isDefault() const
{ return _default; }

bool DbPatternImpl::userVisible() const
{ return _visible; }

TranslatedText DbPatternImpl::category() const
{ return _category; }

Pathname DbPatternImpl::icon() const
{ return _icon; }

Pathname DbPatternImpl::script() const
{ return _script; }
      
Label DbPatternImpl::order() const
{ return _order; }

ZmdId DbPatternImpl::zmdid() const
{ return _zmdid; }


  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
