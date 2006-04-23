/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/dbsource/DbScriptImpl.h
 *
*/

#include <fstream>

#include "DbScriptImpl.h"
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
//        CLASS NAME : DbScriptImpl
//
///////////////////////////////////////////////////////////////////

/** Default ctor
*/
DbScriptImpl::DbScriptImpl (Source_Ref source_r, std::string do_script, std::string undo_script, ZmdId zmdid)
    : _source( source_r )
    , _do_script( do_script )
    , _undo_script( undo_script )
    , _zmdid( zmdid )
{
}

Source_Ref
DbScriptImpl::source() const
{ return _source; }

Pathname
DbScriptImpl::do_script() const
{
    if (_do_script.empty()) {
	return Pathname();
    }
    _tmp_file = filesystem::TmpFile();
    Pathname path = _tmp_file.path();
    ofstream st( path.asString().c_str() );
    st << _do_script << endl;
    return path;
}

Pathname
DbScriptImpl::undo_script() const
{
    if (_undo_script.empty()) {
	return Pathname();
    }
    _tmp_file = filesystem::TmpFile();
    Pathname path = _tmp_file.path();
    ofstream st( path.asString().c_str() );
    st << _undo_script << endl;
    return path;
}

bool
DbScriptImpl::undo_available() const
{
    return !_undo_script.empty();
}

ZmdId DbScriptImpl::zmdid() const
{ return _zmdid; }

  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
