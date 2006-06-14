#
# Makefile.am for zmd/backend/dbsource
#

INCLUDES = 					\
	-I$(top_srcdir) $(ZYPP_CFLAGS) -Wall	\
	-DG_LOG_DOMAIN=\"dbsource\"		\
	-DZYPP_BASE_LOGGER_LOGGROUP=\"dbsource\"

pkginclude_HEADERS =				\
	DbSources.h				\
	DbAccess.h				\
	utils.h					\
	zmd-backend.h

noinst_HEADERS =				\
	DbSourceImpl.h				\
	DbPackageImpl.h				\
	DbAtomImpl.h				\
	DbScriptImpl.h				\
	DbMessageImpl.h				\
	DbLanguageImpl.h			\
	DbPatchImpl.h				\
	DbPatternImpl.h				\
	DbProductImpl.h

lib_LTLIBRARIES =	lib@PACKAGE@.la

lib@PACKAGE@_la_SOURCES =			\
	DbSourceImpl.cc				\
	DbPackageImpl.cc			\
	DbAtomImpl.cc				\
	DbScriptImpl.cc				\
	DbMessageImpl.cc			\
	DbLanguageImpl.cc			\
	DbPatchImpl.cc				\
	DbPatternImpl.cc			\
	DbProductImpl.cc			\
	DbSources.cc				\
	DbAccess.cc 				\
	utils.cc				\
	zmd-backend.cc

lib@PACKAGE@_la_LDFLAGS =	@BACKEND_VERSION_INFO@

#<INSTALL-HEADER-TARGET>

# Automatically inserted by insert_install_target
# DO NOT REMOVE THE TAGS!

# Install header files only if different to version already installed -
# prevent unnecessary installations to keep make dependency changes
# and thus rebuilds for dependent packages as low as possible.

install-pkgincludeHEADERS: $(pkginclude_HEADERS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pkgincludedir)
	@list='$(pkginclude_HEADERS)';							\
	for p in $$list; do								\
		if test -f "$$p"; then d= ; else d="$(srcdir)/"; fi; 			\
		if ! cmp -s $$d$$p $(DESTDIR)$(pkgincludedir)/$$p; then			\
			echo " $(INSTALL_DATA) $$d$$p $(DESTDIR)$(pkgincludedir)/$$p";	\
			$(INSTALL_DATA) $$d$$p $(DESTDIR)$(pkgincludedir)/$$p;		\
	  	fi									\
	done

#</INSTALL-HEADER-TARGET>

