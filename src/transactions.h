/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <map>

#include "dbsource/DbSources.h"
#include "zypp/ResPool.h"
#include "zypp/solver/detail/ResolverContext.h"
#include <sqlite3.h>

typedef std::map<int, zypp::PoolItem_Ref> IdItemMap;


extern int read_transactions( const zypp::ResPool & pool, sqlite3 *db, const DbSources & sources, int & removals, IdItemMap & items, bool & have_best_package );
extern bool write_transactions (const zypp::ResPool & pool, sqlite3 *db, zypp::solver::detail::ResolverContext_Ptr context);
extern void drop_transaction (sqlite3 *db, sqlite_int64 id);

