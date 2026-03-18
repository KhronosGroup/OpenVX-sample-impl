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
set( TARGET_NAME openvx )

include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${SAMPLE_HEADER_DIR}
                     ${VX_HEADER_DIR}
                     ${CMAKE_SOURCE_DIR}/utils
                     ${CMAKE_SOURCE_DIR}/debug
if (OPENVX_CONFORMANCE_NNEF_IMPORT)
                     ${CMAKE_SOURCE_DIR}/kernels/NNEF-Tools/parser/cpp/include
endif (OPENVX_CONFORMANCE_NNEF_IMPORT)
                     ${LIBXML2_INCLUDE_DIR})

if (OPENVX_USE_OPENCL_INTEROP)
    include_directories (${OpenCL_INCLUDE_DIRS} ${OpenCL_INCLUDE_DIRS}/Headers)
endif (OPENVX_USE_OPENCL_INTEROP)
					 
if ((WIN32) OR (CYGWIN))
   add_definitions( ${OPENVX_BUILDING_EXPORT_DEF} )
endif ((WIN32) OR (CYGWIN))
					 
FIND_SOURCES()

if ((WIN32) OR (CYGWIN))
    set( DEF_FILE openvx.def )
endif ((WIN32) OR (CYGWIN))

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME} SHARED
    ${SOURCE_FILES}
    ${DEF_FILE}
    ${LIST_OF_SAMPLE_HEADER_FILES}
    ${LIST_OF_VX_HEADER_FILES}
    )

set( LINK_ADDITIONAL )

if (NOT WIN32)
    list( APPEND LINK_ADDITIONAL dl m )
	if (ANDROID)
	    list( APPEND LINK_ADDITIONAL log )
	else (ANDROID)
	  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	    list( APPEND LINK_ADDITIONAL pthread )
	  else()
	    list( APPEND LINK_ADDITIONAL pthread rt )
	  endif()
	endif (ANDROID)
endif (NOT WIN32)

if (OPENVX_USE_XML)
    list( APPEND LINK_ADDITIONAL ${LIBXML2_LIBRARIES} )
endif (OPENVX_USE_XML)

if (OPENVX_USE_OPENCL_INTEROP)
    list( APPEND LINK_ADDITIONAL ${OpenCL_LIBRARIES})
endif (OPENVX_USE_OPENCL_INTEROP)

if (CYGWIN)
    set_target_properties( ${TARGET_NAME} PROPERTIES LINK_FLAGS ${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE} )
endif (CYGWIN)

if (OPENVX_CONFORMANCE_NNEF_IMPORT)
target_link_libraries( ${TARGET_NAME} openvx-helper utils-lib vx_debug nnef-lib ${LINK_ADDITIONAL} )
else ()
target_link_libraries( ${TARGET_NAME} openvx-helper utils-lib vx_debug ${LINK_ADDITIONAL} )
endif (OPENVX_CONFORMANCE_NNEF_IMPORT)

install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
		  
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${SAMPLE_FOLDER} )
