/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* DbSources.cc  wrapper for zmd.db 'catalogs' table
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

#include <iostream>

#include "zypp/base/Logger.h"
#include "zypp/base/Exception.h"

#include "zypp/Source.h"
#include "zypp/SourceFactory.h"
#include "zypp/SourceManager.h"
#include "zypp/source/SourceImpl.h"

#include "zypp/media/MediaManager.h"

#include "DbSources.h"
#include "DbSourceImpl.h"
#include <sqlite3.h>

using namespace std;
using namespace zypp;

//---------------------------------------------------------------------------

DbSources::DbSources (sqlite3 *db)
    : _db (db)
{ 
    MIL << "DbSources::DbSources(" << db << ")" << endl;
}

DbSources::~DbSources ()
{
}


ResObject::constPtr
DbSources::getById (sqlite_int64 id) const
{
    IdMap::const_iterator it = _idmap.find(id);
    if (it == _idmap.end())
	return NULL;
    return it->second;
}


Source_Ref
DbSources::createDummy( const Url & url, const string & catalog )
{
    media::MediaManager mmgr;
    media::MediaId mediaid = mmgr.open( url );
    SourceFactory factory;

    try {

	DbSourceImpl *impl = new DbSourceImpl ();
	impl->factoryCtor( mediaid, Pathname(), catalog );
	impl->setId( catalog );
	impl->setZmdName( catalog );
	impl->setZmdDescription ( catalog );
	impl->setPriority( 0 );
	impl->setSubscribed( true );

	Source_Ref src( factory.createFrom( impl ) );
	return src;
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT(excpt_r);
    }

    return Source_Ref();
}


const SourcesList &
DbSources::sources (bool refresh)
{
    MIL << "DbSources::sources(" << (refresh ? "refresh" : "") << ")" << endl;

    if (_db == NULL)
	return _sources;

    if (!refresh
	&& !_sources.empty())
    {
	return _sources;
    }

    _sources.clear();

    const char *query =
	//      0   1     2      3            4         5
	"SELECT id, name, alias, description, priority, subscribed "
	"FROM catalogs";

    sqlite3_stmt *handle = NULL;
    int rc = sqlite3_prepare (_db, query, -1, &handle, NULL);
    if (rc != SQLITE_OK) {
	ERR << "Can not read 'channels': " << sqlite3_errmsg (_db) << endl;
	return _sources;
    }

    media::MediaManager mmgr;
    media::MediaId mediaid = mmgr.open( Url( "file:/" ) );
    SourceFactory factory;

    // read catalogs table

    while ((rc = sqlite3_step (handle)) == SQLITE_ROW) {
	const char *text = (const char *) sqlite3_column_text( handle, 0 );
	if (text == NULL) {
	    ERR << "Catalog id is NULL" << endl;
	    continue;
	}
	string id (text);
	string name;
	text = (const char *) sqlite3_column_text( handle, 1 );
	if (text != NULL) name = text;
	string alias;
	text = (const char *) sqlite3_column_text( handle, 2 );
	if (text != NULL) alias = text;
	string desc;
	text = (const char *) sqlite3_column_text( handle, 3 );
	if (text != NULL) desc = text;
	unsigned priority = sqlite3_column_int( handle, 4 );
	int subscribed = sqlite3_column_int( handle, 5 );

	MIL << "id " << id
	    << ", name " << name
	    << ", alias " << alias
	    << ", desc " << desc
	    << ", prio " << priority
	    << ", subs " << subscribed
	    << endl;

	if (alias.empty()) alias = name;
	if (desc.empty()) desc = alias;

	try {

	    DbSourceImpl *impl = new DbSourceImpl ();
	    impl->factoryCtor( mediaid, Pathname(), alias );
	    impl->setId( id );
	    impl->setZmdName( name );
	    impl->setZmdDescription ( desc );
	    impl->setPriority( priority );
	    impl->setSubscribed( subscribed != 0 );

	    impl->attachDatabase( _db );
	    impl->attachIdMap( &_idmap );

	    Source_Ref src( factory.createFrom( impl ) );
	    _sources.push_back( src );
	}
	catch (Exception & excpt_r) {
	    ZYPP_CAUGHT(excpt_r);
	}

    }

    if (rc != SQLITE_DONE) {
	ERR << "Error while reading 'channels': " << sqlite3_errmsg (_db) << endl;
	_sources.clear();
    }

    MIL << "Read " << _sources.size() << " catalogs" << endl;
    return _sources;
}
