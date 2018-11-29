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


include_directories( BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${VX_HEADER_DIR}
    )
					 
# set target name
set( TARGET_NAME_XYZ_LIB vx_xyz_lib )
# set target name
set( TARGET_NAME_XYZ xyz )
# set target name
set( TARGET_NAME_EXAMPLE vx_example )
# set target name
set( TARGET_NAME_EXAMPLE_CODE vx_example_code )
if (OPENVX_USE_TILING)
    # set target name
    set( TARGET_NAME_EXAMPLE_TILING openvx-tiling )
    # set target name
    set( TARGET_NAME_TILING_TEST tiling_test )
endif (OPENVX_USE_TILING)

if ((WIN32) OR (CYGWIN))
   add_definitions( ${OPENVX_BUILDING_EXPORT_DEF} )
endif ((WIN32) OR (CYGWIN))

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME_XYZ_LIB}  vx_xyz_lib.c)

target_link_libraries( ${TARGET_NAME_XYZ_LIB} openvx )

if ((WIN32) OR (CYGWIN))
   set( DEF_FILE xyz.def )
endif ((WIN32) OR (CYGWIN))
# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME_XYZ} SHARED vx_xyz_module.c ${DEF_FILE})

if (CYGWIN)
    set_target_properties( ${TARGET_NAME_XYZ} PROPERTIES LINK_FLAGS ${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE} )
endif (CYGWIN)

target_link_libraries( ${TARGET_NAME_XYZ} openvx )

# add a target named ${TARGET_NAME}
add_executable (${TARGET_NAME_EXAMPLE} vx_graph_factory.c vx_factory_corners.c vx_factory_pipeline.c vx_factory_edge.c)

target_link_libraries( ${TARGET_NAME_EXAMPLE} vx_xyz_lib openvx-debug-lib openvx-helper openvx )

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME_EXAMPLE_CODE}  vx_imagepatch.c vx_delaygraph.c vx_super_res.c  vx_independent.c vx_matrix_access.c vx_parameters.c vx_kernels.c vx_single_node_graph.c vx_introspection.c vx_multi_node_graph.c vx_convolution.c vx_warps.c vx_callback.c vx_extensions.c)

if (OPENVX_USE_TILING)
    # add a target named ${TARGET_NAME}
    add_library (${TARGET_NAME_EXAMPLE_TILING} SHARED vx_tiling_add.c vx_tiling_alpha.c vx_tiling_gaussian.c vx_tiling_box.c vx_tiling_ext.c)

    target_link_libraries( ${TARGET_NAME_EXAMPLE_TILING} openvx )

    # add a target named ${TARGET_NAME}
    add_executable (${TARGET_NAME_TILING_TEST} vx_tiling_main.c)

    target_link_libraries( ${TARGET_NAME_TILING_TEST} openvx-debug-lib openvx-helper openvx )
endif (OPENVX_USE_TILING)

install ( TARGETS ${TARGET_NAME_XYZ_LIB} ${TARGET_NAME_XYZ} ${TARGET_NAME_EXAMPLE} ${TARGET_NAME_EXAMPLE_CODE} ${TARGET_NAME_EXAMPLE_TILING} ${TARGET_NAME_TILING_TEST}
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION bin
          LIBRARY DESTINATION bin )
		  
set_target_properties( ${TARGET_NAME_XYZ_LIB} PROPERTIES FOLDER ${EXAMPLES_FOLDER} )
set_target_properties( ${TARGET_NAME_XYZ} PROPERTIES FOLDER ${EXAMPLES_FOLDER} )
set_target_properties( ${TARGET_NAME_EXAMPLE} PROPERTIES FOLDER ${EXAMPLES_FOLDER} )
set_target_properties( ${TARGET_NAME_EXAMPLE_CODE} PROPERTIES FOLDER ${EXAMPLES_FOLDER} )
set_target_properties( ${TARGET_NAME_EXAMPLE_TILING} PROPERTIES FOLDER ${EXAMPLES_FOLDER} )
set_target_properties( ${TARGET_NAME_TILING_TEST} PROPERTIES FOLDER ${EXAMPLES_FOLDER} )
