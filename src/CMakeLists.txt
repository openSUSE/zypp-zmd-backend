ADD_SUBDIRECTORY( dbsource )

SET( zypp_sources_SRCS
  zypp-sources.cc
)
ADD_EXECUTABLE( zypp-sources ${zypp_sources_SRCS} )
TARGET_LINK_LIBRARIES( zypp-sources zmd-backend )


SET( service_delete_SRCS 
  service-delete.cc
)
ADD_EXECUTABLE( service-delete ${service_delete_SRCS} )
TARGET_LINK_LIBRARIES( service-delete zmd-backend )


SET( query_pool_SRCS
  query-pool.cc 
)
ADD_EXECUTABLE( query-pool ${query_pool_SRCS} )
TARGET_LINK_LIBRARIES( query-pool zmd-backend )


SET ( resolve_dependencies_SRCS
  resolve-dependencies.cc
  transactions.cc
  transactions.h
  locks.cc
  locks.h
)
ADD_EXECUTABLE( resolve-dependencies ${resolve_dependencies_SRCS} )
TARGET_LINK_LIBRARIES( resolve-dependencies zmd-backend )


SET( transact_SRCS
  transact.cc
  transactions.cc
  transactions.h
  RpmCallbacks.h
  MediaChangeCallback.h
  MessageResolvableReportCallback.h
  KeyRingCallbacks.h 
  locks.cc
  locks.h
)
ADD_EXECUTABLE( transact ${transact_SRCS} )
TARGET_LINK_LIBRARIES( transact zmd-backend )

SET( parse_metadata_SRCS
  parse-metadata.cc
  KeyRingCallbacks.h
)
ADD_EXECUTABLE( parse-metadata ${parse_metadata_SRCS} )
TARGET_LINK_LIBRARIES( parse-metadata zmd-backend )

SET( update_status_SRCS
  update-status.cc
  locks.cc
  locks.h
)
ADD_EXECUTABLE( update-status ${update_status_SRCS} )
TARGET_LINK_LIBRARIES( update-status zmd-backend )

INSTALL( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/service-delete ${CMAKE_CURRENT_BINARY_DIR}/query-pool ${CMAKE_CURRENT_BINARY_DIR}/resolve-dependencies ${CMAKE_CURRENT_BINARY_DIR}/transact ${CMAKE_CURRENT_BINARY_DIR}/parse-metadata ${CMAKE_CURRENT_BINARY_DIR}/update-status DESTINATION ${LIB_INSTALL_DIR}/zmd )
