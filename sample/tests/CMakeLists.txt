#
# Copyright (c) 2011-2018 The Khronos Group Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


# set target name
set( TARGET_NAME vx_test )

include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${VX_HEADER_DIR}
                     )
					 
# add a target named ${TARGET_NAME}
add_executable (${TARGET_NAME}  vx_test.c)

target_link_libraries( ${TARGET_NAME} vx_xyz_lib openvx-debug-lib openvx-extras-lib openvx-helper openvx vxu )

install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
		  
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${SAMPLE_FOLDER} )

set( TARGET_NAME2 vx_bug13510 )
add_executable ( ${TARGET_NAME2} vx_bug13510.c)
target_link_libraries( ${TARGET_NAME2} openvx-debug-lib openvx-extras-lib openvx-helper openvx vxu )
install ( TARGETS ${TARGET_NAME2} 
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
set_target_properties( ${TARGET_NAME2} PROPERTIES FOLDER ${SAMPLE_FOLDER} )

set( TARGET_NAME3 vx_bug13518 )
add_executable ( ${TARGET_NAME3} vx_bug13518.c)
target_link_libraries( ${TARGET_NAME3} openvx-debug-lib openvx-extras-lib openvx-helper openvx vxu )
install ( TARGETS ${TARGET_NAME3} 
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
set_target_properties( ${TARGET_NAME3} PROPERTIES FOLDER ${SAMPLE_FOLDER} )

set( TARGET_NAME4 vx_bug13517 )
add_executable ( ${TARGET_NAME4} vx_bug13517.c)
target_link_libraries( ${TARGET_NAME4} openvx-debug-lib openvx-extras-lib openvx-helper openvx vxu )
install ( TARGETS ${TARGET_NAME4} 
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
set_target_properties( ${TARGET_NAME4} PROPERTIES FOLDER ${SAMPLE_FOLDER} )
		  
if ((UNIX) AND (OPENVX_USE_SDL))
    
	set( TARGET_NAME_CAM_TEST vx_cam_test )

	# add a target named ${TARGET_NAME}
	add_executable (${TARGET_NAME_CAM_TEST}  vx_cam_test.c)

	target_link_libraries( ${TARGET_NAME_CAM_TEST} openvx-debug-lib openvx-extras-lib openvx-helper openvx vxu )

	install ( TARGETS ${TARGET_NAME_CAM_TEST} 
			  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
			  
	set_target_properties( ${TARGET_NAME_CAM_TEST} PROPERTIES FOLDER ${SAMPLE_FOLDER} )

endif ((UNIX) AND (OPENVX_USE_SDL))
