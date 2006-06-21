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

#include "zmd-backend.h"
#include "utils.h"
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


//
// actually create sources from catalogs table
// if zypp_restore == true, match catalogs entries with actual zypp sources
//   and return zypp source on match
//   this is needed by the 'transact' helper
//
// else create dummy file:/ source since a real source is either not needed
//   or files are provided by zmd
//

const SourcesList &
DbSources::sources( bool zypp_restore, bool refresh )
{
    MIL << "DbSources::sources(" << (zypp_restore ? "zypp_restore " : "") << (refresh ? "refresh" : "") << ")" << endl;

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
	cerr << "1|Can not read 'channels': " << sqlite3_errmsg (_db) << endl;
	ERR << "Can not read 'channels': " << sqlite3_errmsg (_db) << endl;
	return _sources;
    }

    media::MediaManager mmgr;
    _smgr = SourceManager::sourceManager();

    // assume all sources in sqlite are 'local' (resp. ZMD owned)
    //  as ZMD does the download of packages to a local cache dir,
    //  libzypp never downloads itself but assumes all packages are local
    //  hence we set the base Url to "file:/" and attach package_filename
    //  (attribute of package_details table) later, giving a complete,
    //  local Url. See #176964
    Url url( "file:/" );
    media::MediaId mediaid = mmgr.open( url );

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

	// try to find a matching YaST source. This is needed for non-YUM type
	// repositories, e.g. CD, DVD or local mounts (nfs, smb, ...)

	Source_Ref zypp_source;

	if (zypp_restore
	    && id[0] != '@')		// not for zmd '@system' and '@local'
	{
	    SourceManager::SourceInfoList SIlist = _smgr->knownSourceInfos( "/" );
	    for (SourceManager::SourceInfoList::const_iterator it = SIlist.begin(); it != SIlist.end(); ++it) {
		MIL << "Try to find name '" << name << "' as zypp source" << endl;
		if (it->alias != name) {
		    MIL << "Try to find alias '" << alias << "' as zypp source" << endl;
		    if (it->alias != alias) {
			// #177543
			MIL << "Try to find URL '" << id << "' as zypp source" << endl;
			if (it->url.asString() != id) {
			    continue;
			}
		    }
		}

		MIL << "Found source: alias '" << it->alias << ", url '" << it->url << "', type '" << it->type << "'" << endl;

		// found it

		// If the source is zypp owned (passed to parse-metadata as type 'zypp' before), use the real zypp source

		if ( backend::isZyppOwned( id ) )
		{
		    MIL << "Use real zypp source" << endl;
		    zypp_source = backend::findSource( _smgr, it->alias, it->url );
		}

		break;
	    }
	}

	// now create the source from the database
	//   and attach the zypp_source from above

	try {

	    DbSourceImpl *impl = new DbSourceImpl ();
	    impl->factoryCtor( mediaid, Pathname(), alias );
	    impl->setId( id );
	    impl->setUrl( url );
	    impl->setZmdName( name );
	    impl->setZmdDescription ( desc );
	    impl->setPriority( priority );
	    impl->setSubscribed( subscribed != 0 );

	    impl->attachDatabase( _db );
	    impl->attachIdMap( &_idmap );
	    impl->attachZyppSource( zypp_source );	// link to the real source if needed

	    Source_Ref src( factory.createFrom( impl ) );
	    _sources.push_back( src );
	    MIL << "Created " << src << endl;
	}
	catch (Exception & excpt_r) {
	    cerr << "2|Can't restore sources: " << joinlines( excpt_r.asUserString() ) << endl;
	    ZYPP_CAUGHT(excpt_r);
	    ERR << "Couldn't create zmd source" << endl;
	}

    }

    if (rc != SQLITE_DONE) {
	ERR << "Error while reading 'channels': " << sqlite3_errmsg (_db) << endl;
	_sources.clear();
    }

    MIL << "Read " << _sources.size() << " catalogs" << endl;
    return _sources;
}
