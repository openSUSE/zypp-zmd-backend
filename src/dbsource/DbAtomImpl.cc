/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbAtomImpl.h
 *
*/

#include "DbAtomImpl.h"
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
//        CLASS NAME : DbAtomImpl
//
///////////////////////////////////////////////////////////////////

/** Default ctor
*/
DbAtomImpl::DbAtomImpl (Source_Ref source_r)
    : _source (source_r)
{
}

Source_Ref
DbAtomImpl::source() const
{ return _source; }

  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
