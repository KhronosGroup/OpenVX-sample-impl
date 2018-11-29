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
set( TARGET_NAME_1 openvx-debug-lib )
set( TARGET_NAME_2 openvx-debug )

include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${VX_HEADER_DIR}
                     ${CMAKE_SOURCE_DIR}/kernels/debug )
					 
if ((WIN32) OR (CYGWIN))
   add_definitions( ${OPENVX_BUILDING_EXPORT_DEF} )
endif ((WIN32) OR (CYGWIN))

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME_1} vx_debug_lib.c)

FIND_SOURCES( vx_debug_lib.c )

if ((WIN32) OR (CYGWIN))
   set( DEF_FILE openvx-debug.def )
endif ((WIN32) OR (CYGWIN))

add_library (${TARGET_NAME_2} SHARED ${SOURCE_FILES} ${DEF_FILE})

if (CYGWIN)
    set_target_properties( ${TARGET_NAME_2} PROPERTIES LINK_FLAGS ${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE} )
endif (CYGWIN)

target_link_libraries( ${TARGET_NAME_2} openvx-helper openvx-debug_k-lib openvx )

install ( TARGETS ${TARGET_NAME_1} ${TARGET_NAME_2}
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION bin
          LIBRARY DESTINATION bin )
		  
set_target_properties( ${TARGET_NAME_1} PROPERTIES FOLDER ${LIBRARIES_DEBUG_FOLDER} )
set_target_properties( ${TARGET_NAME_2} PROPERTIES FOLDER ${LIBRARIES_DEBUG_FOLDER} )
