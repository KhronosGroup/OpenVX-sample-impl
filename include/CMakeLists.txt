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

# Merge the two include files in a fingle VX directory
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/VX DESTINATION include )
install( DIRECTORY ${CMAKE_SOURCE_DIR}/api-docs/include/VX DESTINATION include )

include_directories(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/api-docs/include
    )

set(OPENVX_STD_HEADER_BASENAMES
    vx.h vxu.h vx_vendors.h vx_types.h vx_kernels.h vx_api.h vx_nodes.h vx_compatibility.h
    )
set(OPENVX_EXT_HEADER_BASENAMES
    vx_import.h vx_khr_class.h vx_khr_icd.h vx_khr_ix.h vx_khr_nn.h vx_khr_tiling.h vx_khr_xml.h
    )

set(OPENVX_STD_HEADERS "")
set(OPENVX_STD_HEADERS_ABSPATH "")
foreach(h ${OPENVX_STD_HEADER_BASENAMES})
  set(OPENVX_STD_HEADERS ${OPENVX_STD_HEADERS} VX/${h})
  set(OPENVX_STD_HEADERS_ABSPATH ${OPENVX_STD_HEADERS_ABSPATH} ${CMAKE_SOURCE_DIR}/include/VX/${h})
endforeach()
set(OPENVX_EXT_HEADERS "")
set(OPENVX_EXT_HEADERS_ABSPATH "")
foreach(h ${OPENVX_EXT_HEADER_BASENAMES})
  set(OPENVX_EXT_HEADERS ${OPENVX_EXT_HEADERS} VX/${h})
  set(OPENVX_EXT_HEADERS_ABSPATH ${OPENVX_EXT_HEADERS_ABSPATH} ${CMAKE_SOURCE_DIR}/include/VX/${h})
endforeach()

set(OPENVX_HEADERS_DESTDIR "${CMAKE_BINARY_DIR}/packages")
set(OPENVX_STD_HEADERS_PACKAGE_NAME "openvx-standard-headers-${OPENVX_VERSION}.tar.bz2")
set(OPENVX_EXT_HEADERS_PACKAGE_NAME "openvx-extension-headers-${OPENVX_VERSION}.tar.bz2")

if (UNIX)
  add_custom_command(
      OUTPUT ${OPENVX_HEADERS_DESTDIR}/${OPENVX_STD_HEADERS_PACKAGE_NAME}
      COMMAND mkdir -p ${OPENVX_HEADERS_DESTDIR} && echo Create package ${OPENVX_HEADERS_DESTDIR}/${OPENVX_STD_HEADERS_PACKAGE_NAME} && tar -cjf ${OPENVX_HEADERS_DESTDIR}/${OPENVX_STD_HEADERS_PACKAGE_NAME} -C ${CMAKE_SOURCE_DIR}/include ${OPENVX_STD_HEADERS}
      DEPENDS ${OPENVX_STD_HEADERS_ABSPATH}
      )
  add_custom_command(
      OUTPUT ${OPENVX_HEADERS_DESTDIR}/${OPENVX_EXT_HEADERS_PACKAGE_NAME}
      COMMAND mkdir -p ${OPENVX_HEADERS_DESTDIR} && echo Create package ${OPENVX_HEADERS_DESTDIR}/${OPENVX_EXT_HEADERS_PACKAGE_NAME} && tar -cjf ${OPENVX_HEADERS_DESTDIR}/${OPENVX_EXT_HEADERS_PACKAGE_NAME} -C ${CMAKE_SOURCE_DIR}/include ${OPENVX_EXT_HEADERS}
      DEPENDS ${OPENVX_EXT_HEADERS_ABSPATH}
      )
  set(_INCLUDE_TO_ALL "")
  if (BUILD_PACKAGES)
    set(_INCLUDE_TO_ALL "ALL")
endif()
add_custom_target(openvx-header-packages ${_INCLUDE_TO_ALL} DEPENDS
    ${OPENVX_HEADERS_DESTDIR}/${OPENVX_STD_HEADERS_PACKAGE_NAME}
    ${OPENVX_HEADERS_DESTDIR}/${OPENVX_EXT_HEADERS_PACKAGE_NAME}
 )
endif()

# Get the list of headers files for the IDE
file( GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/VX/*.h ${CMAKE_SOURCE_DIR}/api-docs/include/VX/*.h)
set( LIST_OF_VX_HEADER_FILES ${HEADER_FILES} PARENT_SCOPE )
set( VX_HEADER_DIR ${CMAKE_SOURCE_DIR}/api-docs/include ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
