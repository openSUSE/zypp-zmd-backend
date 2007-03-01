PROJECT(libzypp-zmd-backend)

####################################################################
# CONFIGURATION                                                    #
####################################################################

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
SET(CMAKE_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/share/cmake/Modules ${CMAKE_MODULE_PATH})

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
