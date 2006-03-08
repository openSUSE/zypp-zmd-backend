dnl Process this file with autoconf to produce a configure script.
dnl ==================================================

AC_INIT(zmd-backend,7.1.1)
AC_CONFIG_SRCDIR(src/Makefile.am)

AC_PREFIX_DEFAULT(/usr)
AC_CANONICAL_TARGET
AC_ARG_PROGRAM

AM_INIT_AUTOMAKE(foreign dist-bzip2 no-dist-gzip tar-ustar)
AM_CONFIG_HEADER(config.h)

dnl ==================================================

CXXFLAGS="${CXXFLAGS} -g -O2 -Wall -Wformat"
AC_SYS_LARGEFILE

dnl ==================================================

docdir=\${datadir}/doc/packages
pkgdocdir=\${docdir}/$PACKAGE

AC_SUBST(docdir)
AC_SUBST(pkgdocdir)

zyppdir=${prefix}/share/$PACKAGE
localedir=${prefix}/share/locale

AC_SUBST(localedir)

AH_TEMPLATE([LOCALEDIR],[gettext localedir])
AC_DEFINE_UNQUOTED([LOCALEDIR], ["${localedir}"])
AH_TEMPLATE([TEXTDOMAIN],[gettext textdomain])
AC_DEFINE_UNQUOTED([TEXTDOMAIN], ["$PACKAGE"])

dnl ==================================================
dnl checks for programs

dnl   check for c++ compiler and tools
AC_PROG_CXX
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AM_PROG_LIBTOOL

dnl   do AC_CHECK_* tests using c++ (e.g. boost header)
AC_LANG([C++])

AC_PATH_PROG(XGETTEXT, xgettext)
if test -z "$XGETTEXT" ; then
    AC_MSG_ERROR(xgettext is missing; please install gettext-devel.)
fi

AC_PATH_PROG(MYRUNTEST, runtest)
if test -z "$MYRUNTEST" ; then
    AC_MSG_ERROR(runtest is missing; please install dejagnu.)
fi

dnl ==================================================
dnl checks for libraries
dnl libzypp
AC_CHECK_HEADER([zypp/ZYpp.h],
        [:],
        AC_MSG_ERROR([ZYpp.h not found. please install libzypp-devel]))

dnl libz
AC_CHECK_LIB([z], [gzopen],
        [:],
        AC_MSG_ERROR([libz not found. please install zlib-devel]))

dnl libxml2
AC_CHECK_LIB([xml2], [xmlNewTextReader],
        [:],
        AC_MSG_ERROR([libxml2 not found. please install libxml2-devel]))

dnl sqlite
AM_CONDITIONAL(BUILD_ZMD, true)
PKG_CHECK_MODULES(SQLITE, sqlite3, pkg_sqlite=yes, pkg_sqlite=no)
if test $pkg_sqlite = no; then
  echo
  AC_MSG_ERROR([Could not find sqlite3 library, please install sqlite-devel])
fi
dnl ==================================================
dnl internationalization macros
AM_GNU_GETTEXT

dnl ==================================================
dnl checks for typedefs
dnl ==================================================
dnl checks for structures
dnl ==================================================
dnl checks for compiler characteristics
dnl ==================================================
dnl checks for library functions
dnl ==================================================
dnl checks for system services
dnl ==================================================
dnl AC_CONFIG_FILES([FILE...])
AC_OUTPUT(Makefile			\
	src/Makefile			\
	testsuite/Makefile		\
	testsuite/tests/Makefile	\
	src/dbsource/Makefile
)
dnl ==================================================

echo "
Configuration:

  PACKAGE:              ${PACKAGE}

  COMPILER:             ${CXX}
  CXXFLAGS:          	${CXXFLAGS}

  PREFIX:               ${prefix}
  LIBDIR:               ${libdir}

"
echo

