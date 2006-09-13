/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* DbSourceImpl.h
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 * Copyright (C) 2005 SUSE Linux Products GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef ZMD_BACKEND_DBSOURCEIMPL_H
#define ZMD_BACKEND_DBSOURCEIMPL_H

#include <iosfwd>
#include <string>

#include "zypp/source/SourceImpl.h"
#include "zypp/media/MediaManager.h"

#include "DbAccess.h"

#include "zypp/Package.h"
#include "zypp/Atom.h"
#include "zypp/Message.h"
#include "zypp/Script.h"
#include "zypp/Language.h"
#include "zypp/Patch.h"
#include "zypp/Product.h"
#include "zypp/Pattern.h"

class DbSourceImplPolicy
{
public:
  DbSourceImplPolicy()
      : _create_dependencies(true)
  {}

  /**
   * For transaction it is not needed 
   * to create dependencies
   */
  bool createDependencies() const
  {
    return _create_dependencies;
  }

  void setCreateDependenciesEnabled( bool enabled )
  {
    _create_dependencies = enabled ;
  }

private:
  bool _create_dependencies;
};

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : DbSourceImpl

class DbSourceImpl : public zypp::source::SourceImpl
{

public:
  /** Default ctor */
  DbSourceImpl( DbSourceImplPolicy policy = DbSourceImplPolicy() );
  ~DbSourceImpl();
private:
  /** Ctor substitute.
   * Actually get the metadata.
   * \throw EXCEPTION on fail
   */
  virtual void factoryInit();

  sqlite3 *_db;
  sqlite3_stmt *_dependency_handle;
  sqlite3_stmt *_message_handle;
  sqlite3_stmt *_script_handle;
  sqlite3_stmt *_patch_handle;
  sqlite3_stmt *_pattern_handle;
  sqlite3_stmt *_product_handle;

  void createPackages(void);
  void createAtoms(void);
  void createMessages(void);
  void createScripts(void);
  void createLanguages(void);
  void createPatches(void);
  void createPatterns(void);
  void createProducts(void);

  /**
   * if policy says so, creates dependencies, otherwise returns a
   * empty dependencies set
   */
  zypp::Dependencies createDependenciesOnPolicy(sqlite_int64 resolvable_id);

  /**
   * creates dependencies
   */
  zypp::Dependencies createDependencies (sqlite_int64 resolvable_id);

public:

  virtual const bool valid() const
  {
    return true;
  }

  virtual std::string type(void) const
  {
    return "ZMD";
  }

  void attachDatabase( sqlite3 *db );
  void attachIdMap (IdMap *idmap);
  void attachZyppSource( zypp::Source_Ref source );

private:
  zypp::Source_Ref _source;		// reference to DbSource for this Impl
  zypp::Source_Ref _zyppsource;	// reference to real zypp source, if exists
  IdMap *_idmap;			// map sqlite resolvable.id to actual objects
  void createResolvables( zypp::Source_Ref source_r );
  DbSourceImplPolicy _policy;
};


#endif // ZMD_BACKEND_DBSOURCEIMPL_H
