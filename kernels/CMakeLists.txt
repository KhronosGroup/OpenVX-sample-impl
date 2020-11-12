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


add_subdirectory( c_model )
add_subdirectory( debug )
add_subdirectory( extras )
if (EXPERIMENTAL_USE_VENUM)
    add_subdirectory( venum )
endif (EXPERIMENTAL_USE_VENUM)
if (OPENVX_USE_TILING)
    add_subdirectory( tiling )
endif (OPENVX_USE_TILING)
if (OPENVX_CONFORMANCE_NNEF_IMPORT)
# set target name
set( TARGET_NAME nnef-lib )

include_directories( BEFORE 
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_SOURCE_DIR}/kernels/NNEF-Tools/parser/cpp/include )

if (UNIX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif (UNIX)

FIND_SOURCES()

file(GLOB OVX_SRC_NNEF_LIB "${CMAKE_SOURCE_DIR}/kernels/NNEF-Tools/parser/cpp/src/*.cpp")
add_library (${TARGET_NAME} STATIC ${OVX_SRC_NNEF_LIB} )

install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION bin
          LIBRARY DESTINATION bin )
		  
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${HELPER_FOLDER} )
endif (OPENVX_CONFORMANCE_NNEF_IMPORT)
