#

# Copyright (c) 2011-2017 The Khronos Group Inc.
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
set( TARGET_NAME openvx-venum-lib )

include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${VX_HEADER_DIR}
                     ${CMAKE_SOURCE_DIR}/utils
                     ${CMAKE_SOURCE_DIR}/debug )
					 
FIND_SOURCES()

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME} STATIC ${SOURCE_FILES})

target_link_libraries( ${TARGET_NAME} openvx )

install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION bin
          LIBRARY DESTINATION bin )
		  
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${KERNELS_FOLDER} )
