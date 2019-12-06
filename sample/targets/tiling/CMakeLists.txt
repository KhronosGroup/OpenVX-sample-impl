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
set( TARGET_NAME openvx-tiling_chaining )

include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../../include
                     ${VX_HEADER_DIR}
                     ${CMAKE_SOURCE_DIR}/kernels/tiling
                     ${CMAKE_SOURCE_DIR}/debug
                     ${CMAKE_SOURCE_DIR}/utils
                     )

FIND_SOURCES()

if ((WIN32) OR (CYGWIN))
    set( DEF_FILE openvx-target.def )
endif ((WIN32) OR (CYGWIN))

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME} SHARED ${SOURCE_FILES} ${DEF_FILE})

if (CYGWIN)
    set_target_properties( ${TARGET_NAME} PROPERTIES LINK_FLAGS ${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE} )
endif (CYGWIN)

target_link_libraries( ${TARGET_NAME} openvx-debug-lib openvx-extras-lib openvx-helper openvx-tiling_chaining-lib openvx vxu half)

install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION lib
          LIBRARY DESTINATION bin )
		  
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${SAMPLE_TARGETS_FOLDER} )
