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
#ifndef ZMD_BACKEND_DBSOURCE_DBLANGUAGEIMPL_H
#define ZMD_BACKEND_DBSOURCE_DBLANGUAGEIMPL_H

#include "zypp/Language.h"
#include "zypp/Source.h"

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//        CLASS NAME : DbLanguageImpl
//
/** Class representing a package
*/
class DbLanguageImpl : public detail::LanguageImplIf
{
public:

	/** Default ctor
	*/
	DbLanguageImpl( Source_Ref source_r, ZmdId zmdid );

	/** */
	virtual Source_Ref source() const;
        /** */
	virtual ZmdId zmdid() const;

protected:
	Source_Ref _source;
	ZmdId _zmdid;

 };
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
#endif // ZMD_BACKEND_DBSOURCE_DBLANGUAGEIMPL_H
