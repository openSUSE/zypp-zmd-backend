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
#ifndef ZMD_BACKEND_DBSOURCE_DBMESSAGEIMPL_H
#define ZMD_BACKEND_DBSOURCE_DBMESSAGEIMPL_H

#include "zypp/detail/MessageImpl.h"
#include "zypp/Source.h"

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//        CLASS NAME : DbMessageImpl
//
/** Class representing a package
*/
class DbMessageImpl : public detail::MessageImplIf
{
public:

	/** Default ctor
	*/
	DbMessageImpl( Source_Ref source_r, TranslatedText text, ZmdId zmdid );

	/** */
	virtual Source_Ref source() const;
	virtual TranslatedText text() const;
	virtual ByteCount size() const;
        /** */
	virtual ZmdId zmdid() const;

protected:
	Source_Ref _source;
	TranslatedText _text;
	ZmdId _zmdid;

 };
  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
#endif // ZMD_BACKEND_DBSOURCE_DBMESSAGEIMPL_H
