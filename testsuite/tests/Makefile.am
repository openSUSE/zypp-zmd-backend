## Process this file with automake to produce Makefile.in
## ##################################################

noinst_PROGRAMS =	\
	catalogs	\
	sources		\
	linebreaks

## ##################################################

LDADD = $(top_srcdir)/src/dbsource/lib@PACKAGE@.la \
	$(ZYPP_LIBS)			\
	$(SQLITE_LIBS)
	
CFLAGS = $(ZYPP_FLAGS) $(SQLITE_CFLAGS)

## ##################################################

catalogs_SOURCES = catalogs.cc

sources_SOURCES = sources.cc

linebreaks_SOURCES = linebreaks.cc ../../src/zmd-backend.cc
