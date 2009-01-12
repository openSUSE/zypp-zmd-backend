#
# Makefile.am for zmd/backend
#
SUBDIRS = dbsource

PKG_MANAGEMENT_LIBS = dbsource/lib@PACKAGE@.la
PKG_MANAGEMENT_CFLAGS = -I$(top_srcdir)

AM_CXXFLAGS = $(PKG_MANAGEMENT_CFLAGS) $(SQLITE_CFLAGS) $(ZYPP_CFLAGS)
LDADD = $(PKG_MANAGEMENT_LIBS) $(SQLITE_LIBS) $(ZYPP_LIBS)

bindir = $(libdir)/zmd

noinst_PROGRAMS = \
	zypp-sources

bin_PROGRAMS =	\
	query-pool		\
	resolve-dependencies	\
	service-delete		\
	parse-metadata		\
	update-status		\
	zmd-solver-testcase \
	zmd-list-locks \
	transact

zypp_sources_SOURCES = zypp-sources.cc
service_delete_SOURCES = service-delete.cc
query_pool_SOURCES = query-pool.cc
resolve_dependencies_SOURCES = resolve-dependencies.cc transactions.cc transactions.h locks.cc locks.h
transact_SOURCES = transact.cc transactions.cc transactions.h RpmCallbacks.h MediaChangeCallback.h MessageResolvableReportCallback.h KeyRingCallbacks.h  locks.cc locks.h
parse_metadata_SOURCES = parse-metadata.cc KeyRingCallbacks.h
update_status_SOURCES = update-status.cc  locks.cc locks.h
zmd_solver_testcase_SOURCES = zmd-solver-testcase.cc transactions.cc transactions.h locks.cc locks.h
zmd_list_locks_SOURCES = zmd-list-locks.cc locks.cc locks.h
