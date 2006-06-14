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
	query-system		\
	query-files		\
	resolve-dependencies	\
	service-delete		\
	package-files		\
	parse-metadata		\
	find-file		\
	update-status		\
	transact

zypp_sources_SOURCES = zypp-sources.cc
service_delete_SOURCES = service-delete.cc
query_pool_SOURCES = query-pool.cc
query_system_SOURCES = query-system.cc
query_files_SOURCES = query-files.cc
resolve_dependencies_SOURCES = resolve-dependencies.cc transactions.cc transactions.h
transact_SOURCES = transact.cc transactions.cc transactions.h RpmCallbacks.h MediaChangeCallback.h MessageResolvableReportCallback.h KeyRingCallbacks.h
parse_metadata_SOURCES = parse-metadata.cc KeyRingCallbacks.h
package_files_SOURCES = package-files.cc
find_file_SOURCES = find-file.cc
update_status_SOURCES = update-status.cc

