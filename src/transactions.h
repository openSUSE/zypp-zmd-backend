/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "dbsource/DbSources.h"
#include "zypp/ResPool.h"
#include "zypp/solver/detail/ResolverContext.h"
#include <sqlite3.h>

extern int read_transactions (const zypp::ResPool & pool, sqlite3 *db, const DbSources & sources, int & removals);
extern bool write_transactions (const zypp::ResPool & pool, sqlite3 *db, zypp::solver::detail::ResolverContext_Ptr context);
