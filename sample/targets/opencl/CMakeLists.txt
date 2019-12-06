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


if ( EXPERIMENTAL_USE_OPENCL )
    # set target name
    set( TARGET_NAME openvx-opencl )

    include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR}
					 ${CMAKE_CURRENT_SOURCE_DIR}/../../include
                     ${VX_HEADER_DIR}
					 ${CMAKE_SOURCE_DIR}/debug
                     ${OPENCL_INCLUDE_PATH} )
					     					 
    set( INVERTED_COMMA "\"" )
	set( CL_SOURCE_DIR ${INVERTED_COMMA}${CMAKE_SOURCE_DIR}/kernels/opencl${INVERTED_COMMA} )
					 
	add_definitions( -DVX_CL_SOURCE_DIR=${CL_SOURCE_DIR} )
			
    FIND_SOURCES()
	
	if ((WIN32) OR (CYGWIN))
		set( DEF_FILE openvx-target.def )
	endif ((WIN32) OR (CYGWIN))

    # add a target named ${TARGET_NAME}
    add_library (${TARGET_NAME} SHARED ${SOURCE_FILES} ${DEF_FILE})
	
    if (CYGWIN)
        set_target_properties( ${TARGET_NAME} PROPERTIES LINK_FLAGS ${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE} )
    endif (CYGWIN)

    target_link_libraries( ${TARGET_NAME} openvx )

    install ( TARGETS ${TARGET_NAME} 
              RUNTIME DESTINATION bin
              ARCHIVE DESTINATION lib
              LIBRARY DESTINATION bin )
		  
endif ( EXPERIMENTAL_USE_OPENCL )
