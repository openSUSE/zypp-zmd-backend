/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbMessageImpl.h
 *
*/

#include "DbMessageImpl.h"
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
//        CLASS NAME : DbMessageImpl
//
///////////////////////////////////////////////////////////////////

/** Default ctor
*/
DbMessageImpl::DbMessageImpl (Source_Ref source_r, TranslatedText text, ZmdId zmdid)
    : _source( source_r )
    , _text( text )
    , _zmdid( zmdid )
{
}

Source_Ref
DbMessageImpl::source() const
{ return _source; }

TranslatedText DbMessageImpl::text() const
{ return _text; }

ByteCount DbMessageImpl::size() const
{ return _text.asString().size(); }


ZmdId DbMessageImpl::zmdid() const
{ return _zmdid; }

  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
