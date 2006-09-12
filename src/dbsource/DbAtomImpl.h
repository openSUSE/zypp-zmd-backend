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
#ifndef ZMD_BACKEND_DBSOURCE_DBATOMIMPL_H
#define ZMD_BACKEND_DBSOURCE_DBATOMIMPL_H

#include "zypp/detail/AtomImpl.h"
#include "zypp/Source.h"

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//        CLASS NAME : DbAtomImpl
//
/** Class representing a package
*/
class DbAtomImpl : public detail::AtomImplIf
{
public:

  /** Default ctor
  */
  DbAtomImpl( Source_Ref source_r, ZmdId zmdid );

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
#endif // ZMD_BACKEND_DBSOURCE_DBATOMIMPL_H
