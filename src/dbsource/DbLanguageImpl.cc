/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbLanguageImpl.h
 *
*/

#include "DbLanguageImpl.h"
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
//        CLASS NAME : DbLanguageImpl
//
///////////////////////////////////////////////////////////////////

/** Default ctor
*/
DbLanguageImpl::DbLanguageImpl (Source_Ref source_r, ZmdId zmdid)
    : _source( source_r )
    , _zmdid( zmdid )
{}

Source_Ref
DbLanguageImpl::source() const
{
  return _source;
}

ZmdId DbLanguageImpl::zmdid() const
{
  return _zmdid;
}

/////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
