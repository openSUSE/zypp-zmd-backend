PROJECT(libzypp-zmd-backend)

####################################################################
# CONFIGURATION                                                    #
####################################################################

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
SET(CMAKE_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/share/cmake/Modules ${CMAKE_MODULE_PATH})

# Library
IF ( DEFINED LIB )
  SET ( LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${LIB}" )
ELSE ( DEFINED  LIB )
  IF ( EXISTS "${CMAKE_INSTALL_PREFIX}/lib64" )
    SET( LIB_SUFFIX "64" )
  ENDIF ( EXISTS "${CMAKE_INSTALL_PREFIX}/lib64" )
  SET ( LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" )
ENDIF ( DEFINED  LIB )
MESSAGE(STATUS "Libraries will be installed in ${LIB_INSTALL_DIR}" )

INCLUDE(${CMAKE_SOURCE_DIR}/VERSION.cmake)
SET ( VERSION "${ZMD_VERSION}_${BACKEND_VERSION}" )

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libzypp backend for ZenWorks daemon")
SET(CPACK_PACKAGE_VENDOR "Novell Inc.")
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt")
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION})
#SET(CPACK_PACKAGE_VERSION_MINOR "3")
#SET(CPACK_PACKAGE_VERSION_PATCH "2")

SET( CPACK_GENERATOR "TBZ2")
SET( CPACK_SOURCE_GENERATOR "TBZ2")
SET( CPACK_SOURCE_PACKAGE_FILE_NAME "zmd-backend-${VERSION}" )

# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
SET (CPACK_SOURCE_IGNORE_FILES
"/CVS/;/.svn/;/.libs/;/.deps/;.swp$;.#;/#;/build/"
"~$"
"\\\\.cvsignore$"
"/package"
"Makefile\\\\.in$"
)

INCLUDE(CPack)

INCLUDE(DefineInstallationPaths)

ADD_DEFINITIONS( -DVERSION=\\\"${VERSION}\\\" )
MESSAGE( STATUS "Looking for modules in: ${CMAKE_MODULE_PATH}" )
FIND_PACKAGE(Boost REQUIRED)
FIND_PACKAGE(Sqlite REQUIRED)
FIND_PACKAGE(Zypp REQUIRED)

INCLUDE_DIRECTORIES( ${ZYPP_INCLUDE_DIR} ${SQLITE_INCLUDE_DIR} )

IF (Boost_FOUND)
  MESSAGE( STATUS "boost found: includes in ${Boost_INCLUDE_DIRS}, library in ${Boost_LIBRARY_DIRS}")
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

MESSAGE(STATUS "version: ${VERSION}")
#yumschemadir=${zyppdir}/schema/yum
#xmlstoreschemadir=${zyppdir}/schema/xmlstore

####################################################################
# RPM SPEC                                                         #
####################################################################

MESSAGE(STATUS "Writing spec file...")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/libzypp-zmd-backend.spec.in.cmake ${CMAKE_BINARY_DIR}/package/libzypp-zmd-backend.spec @ONLY)

####################################################################
# SUBDIRECTORIES                                                   #
####################################################################

ADD_SUBDIRECTORY( src ) 

####################################################################
# INCLUDES                                                         #
####################################################################

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

ADD_CUSTOM_TARGET( svncheck
  COMMAND cd $(CMAKE_SOURCE_DIR) && ! LC_ALL=C svn status --show-updates --quiet | grep -v '^Status against revision'
)

SET( AUTOBUILD_COMMAND
  COMMAND ${CMAKE_MAKE_PROGRAM} package_source
  COMMAND ${CMAKE_COMMAND} -E copy ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.bz2 ${CMAKE_BINARY_DIR}/package
  COMMAND ${CMAKE_COMMAND} -E remove ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.bz2
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/package/${PACKAGE}.changes" "${CMAKE_BINARY_DIR}/package/${PACKAGE}.changes"
)

ADD_CUSTOM_TARGET( autobuild_local
  ${AUTOBUILD_COMMAND}
)

ADD_CUSTOM_TARGET( autobuild
  COMMAND ${CMAKE_MAKE_PROGRAM} svncheck
  ${AUTOBUILD_COMMAND}
)