#
# Makefile.am for zmd/backend
#
SUBDIRS = dbsource

PKG_MANAGEMENT_LIBS = dbsource/lib@PACKAGE@.la
PKG_MANAGEMENT_CFLAGS = -I$(top_srcdir)

AM_CXXFLAGS = $(PKG_MANAGEMENT_CFLAGS) $(SQLITE_CFLAGS) $(ZYPP_CFLAGS)
LDADD = $(PKG_MANAGEMENT_LIBS) $(SQLITE_LIBS) $(ZYPP_LIBS)

noinst_HEADERS = zmd-backend.h

bindir = $(libdir)/zmd

noinst_PROGRAMS = \
	zypp-sources

bin_PROGRAMS =	\
	query-pool		\
	query-system		\
	query-files		\
	resolve-dependencies	\
	service-delete		\
	package-files		\
	parse-metadata		\
	find-file		\
	update-status		\
	transact

zypp_sources_SOURCES = zypp-sources.cc zmd-backend.cc
service_delete_SOURCES = service-delete.cc zmd-backend.cc
query_pool_SOURCES = query-pool.cc zmd-backend.cc
query_system_SOURCES = query-system.cc zmd-backend.cc
query_files_SOURCES = query-files.cc zmd-backend.cc
resolve_dependencies_SOURCES = resolve-dependencies.cc transactions.cc transactions.h zmd-backend.cc
transact_SOURCES = transact.cc transactions.cc transactions.h RpmCallbacks.h MediaChangeCallback.h MessageResolvableReportCallback.h KeyRingCallbacks.h zmd-backend.cc
parse_metadata_SOURCES = parse-metadata.cc zmd-backend.cc KeyRingCallbacks.h
package_files_SOURCES = package-files.cc zmd-backend.cc
find_file_SOURCES = find-file.cc zmd-backend.cc
update_status_SOURCES = update-status.cc zmd-backend.cc

