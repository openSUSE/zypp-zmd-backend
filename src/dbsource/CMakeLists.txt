
SET( dbsource_SRCS
  DbAccess.cc
  DbAtomImpl.cc
  DbLanguageImpl.cc
  DbMessageImpl.cc
  DbPackageImpl.cc
  DbPatchImpl.cc 
  DbPatternImpl.cc
  DbProductImpl.cc
  DbScriptImpl.cc
  DbSourceImpl.cc
  DbSources.cc
  utils.cc
  zmd-backend.cc
)

ADD_LIBRARY(zmd-backend SHARED ${dbsource_SRCS})
SET_TARGET_PROPERTIES( zmd-backend PROPERTIES VERSION "0.0.0" )

# System libraries
TARGET_LINK_LIBRARIES(zmd-backend ${ZYPP_LIBRARY} )
TARGET_LINK_LIBRARIES(zmd-backend ${SQLITE_LIBRARY} )


SET( dbsource_HEADERS
  DbAccess.h
  zmd-backend.h
  utils.h
)

SET( dbsource_NOINSTHEADERS
  DbLanguageImpl.h
  DbPackageImpl.h 
  DbPatternImpl.h  
  DbScriptImpl.h  
  DbSources.h  
  DbAtomImpl.h  
  DbMessageImpl.h   
  DbPatchImpl.h
  DbProductImpl.h
  DbSourceImpl.h 
)

INSTALL( FILES ${dbsource_HEADERS} DESTINATION "${CMAKE_INSTALL_PREFIX}/include/zmd-backend" )

MESSAGE( STATUS "uhm: ${LIB_INSTALL_DIR}" )
INSTALL(TARGETS zmd-backend LIBRARY DESTINATION ${LIB_INSTALL_DIR} )
