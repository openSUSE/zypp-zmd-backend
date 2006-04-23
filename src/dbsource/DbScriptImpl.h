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
#ifndef ZMD_BACKEND_DBSOURCE_DBSCRIPTIMPL_H
#define ZMD_BACKEND_DBSOURCE_DBSCRIPTIMPL_H

#include "zypp/detail/ScriptImpl.h"
#include "zypp/TmpPath.h"
#include "zypp/Source.h"

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//        CLASS NAME : DbScriptImpl
//
/** Class representing a package
*/
class DbScriptImpl : public detail::ScriptImplIf
{
public:

	/** Default ctor
	*/
	DbScriptImpl( Source_Ref source_r, std::string do_script, std::string undo_script, ZmdId zmdid );

	/** */
	virtual Source_Ref source() const;
	virtual Pathname do_script() const;
	virtual Pathname undo_script() const;
	virtual bool undo_available() const;

        /** */
	virtual ZmdId zmdid() const;

protected:
	Source_Ref _source;
	std::string _do_script;
	std::string _undo_script;
	ZmdId _zmdid;

	mutable filesystem::TmpFile _tmp_file;
 };
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
#endif // ZMD_BACKEND_DBSOURCE_DBSCRIPTIMPL_H
