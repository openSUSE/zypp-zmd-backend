#
# Makefile.am for zmd/backend/dbsource
#

INCLUDES = 					\
	-I$(top_srcdir) -Wall			\
	-DG_LOG_DOMAIN=\"dbsource\"		\
	-DZYPP_BASE_LOGGER_LOGGROUP=\"dbsource\"

LDADD =	$(noinst_LTLIBRARIES) $(top_srcdir)/zypp/lib@PACKAGE@.la

noinst_HEADERS =				\
	DbSources.h				\
	DbSourceImpl.h				\
	DbPackageImpl.h				\
	DbAtomImpl.h				\
	DbPatchImpl.h				\
	DbPatternImpl.h				\
	DbProductImpl.h				\
	DbAccess.h
	
noinst_LTLIBRARIES =	lib@PACKAGE@_dbsource.la

lib@PACKAGE@_dbsource_la_SOURCES =		\
	DbSourceImpl.cc				\
	DbPackageImpl.cc			\
	DbAtomImpl.cc				\
	DbPatchImpl.cc				\
	DbPatternImpl.cc			\
	DbProductImpl.cc			\
	DbSources.cc				\
	DbAccess.cc 
