## Process this file with automake to produce Makefile.in
## ##################################################

noinst_PROGRAMS =	\
	catalogs	\
	sources

## ##################################################

LDADD = $(top_srcdir)/src/dbsource/lib@PACKAGE@_dbsource.la \
	-lzypp			\
	$(SQLITE_LIBS)
	
CFLAGS = $(SQLITE_CFLAGS)

## ##################################################

catalogs_SOURCES = catalogs.cc

sources_SOURCES = sources.cc


