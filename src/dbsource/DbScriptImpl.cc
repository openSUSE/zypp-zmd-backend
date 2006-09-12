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

#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>

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
{}

Source_Ref
DbScriptImpl::source() const
{
  return _source;
}

Pathname
DbScriptImpl::do_script() const
{
  if ( ! _do_script.empty() )
    return Pathname();

  if ( !_tmp_do_script )
    _tmp_do_script.reset(new filesystem::TmpFile(zypp::getZYpp()->tmpPath(), "zmd-backend-do-script-"));

  Pathname pth = _tmp_do_script->path();
  // FIXME check success
  ofstream st(pth.asString().c_str());

  if ( !st )
  {
    //ZYPP_THROW(Exception(N_("Can't write the patch script to a temporary file.")));
    ERR << "Can't write the patch script to a temporary file." << std::endl;
    return Pathname();
  }
  st << _do_script << endl;
  st.close();
  return pth;
}

Pathname
DbScriptImpl::undo_script() const
{
  if ( ! _undo_script.empty() )
    return Pathname();

  if ( !_tmp_undo_script )
    _tmp_undo_script.reset(new filesystem::TmpFile(zypp::getZYpp()->tmpPath(), "zmd-backend-undo-script-"));

  Pathname pth = _tmp_undo_script->path();
  // FIXME check success
  ofstream st(pth.asString().c_str());

  if ( !st )
  {
    //ZYPP_THROW(Exception(N_("Can't write the patch script to a temporary file.")));
    ERR << "Can't write the patch script to a temporary file." << std::endl;
    return Pathname();
  }

  st << _undo_script << endl;
  st.close();
  return pth;
}

bool
DbScriptImpl::undo_available() const
{
  return !_undo_script.empty();
}

ZmdId DbScriptImpl::zmdid() const
{
  return _zmdid;
}

/////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
