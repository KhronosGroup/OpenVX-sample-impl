include_directories( BEFORE
					 ${CMAKE_SOURCE_DIR}/include )
					 
# set target name
set( TARGET_NAME half )

FIND_SOURCES()



link_directories( ${CMAKE_CURRENT_SOURCE_DIR}  )

add_definitions( ${OPENVX_BUILDING_EXPORT_DEF} )

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME}  ${SOURCE_FILES} )


install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION bin
          LIBRARY DESTINATION bin )

