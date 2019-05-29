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

#
# Global setup file for OpenVX CMake

#
cmake_minimum_required(VERSION 2.8.9)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" OPENVX_VERSION)
string(STRIP "${OPENVX_VERSION}" OPENVX_VERSION)
set(OPENVX_VERSION ${OPENVX_VERSION} CACHE INTERNAL "" FORCE)

message(STATUS "===== OpenVX version: ${OPENVX_VERSION} =====")

# CMAKE variables
set( CMAKE_MODULE_PATH
     ${CMAKE_MODULE_PATH}
     "${CMAKE_SOURCE_DIR}/cmake_utils/"
)

include( cmake_utils/CMakeFuncs.txt )

if (WIN32)
    include( CMake_windows_tools )
elseif (ANDROID)
    include( CMake_android_tools )
else ()
    include( CMake_linux_tools )
endif ()

project (OpenVX)

# Supported configurations
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
  )
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Available build configurations." FORCE)

# Default options for the CMake GUI
option( OPENVX_USE_IX ON )
option( OPENVX_USE_NN ON )
option( OPENVX_USE_S16 OFF )
option( OPENVX_USE_TILING OFF )
option( OPENVX_USE_XML OFF )
option( EXPERIMENTAL_USE_DOT OFF )
option( EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT OFF )
option( EXPERIMENTAL_USE_VENUM OFF)

if (UNIX OR ANDROID)
    add_definitions( -D_XOPEN_SOURCE=700 -D_GNU_SOURCE=1 )
#    set( EXPERIMENTAL_USE_DOT ON )
endif (UNIX OR ANDROID)

message( "" )
message( "** ** ** Selected Extensions ** ** **")
add_definitions( -DOPENVX_BUILDING )

# Official extensions
if (OPENVX_USE_IX)
    add_definitions( -DOPENVX_USE_IX )
    message( "-- Offical Import-Export extension")
endif (OPENVX_USE_IX)
if (OPENVX_USE_NN)
    add_definitions( -DOPENVX_USE_NN )
    message( "-- Offical NeuralNet extension")
endif (OPENVX_USE_NN)
# Provisional extensions
if (OPENVX_USE_TILING)
    add_definitions( -DOPENVX_USE_TILING )
    message( "-- Provisional tiling extension")
endif (OPENVX_USE_TILING)
if (OPENVX_USE_S16)
    add_definitions( -DOPENVX_USE_S16 )
    message( "-- Provisional S16 extension")
endif (OPENVX_USE_S16)
if (OPENVX_USE_XML)
    add_definitions( -DOPENVX_USE_XML )
    message( "-- Provisional 'xml' extension")
endif (OPENVX_USE_XML)
# Experimental extensions
if (OPENVX_USE_SMP)
    add_definitions( -DOPENVX_USE_SMP )
    message( "-- Experimental smp extension")
endif (OPENVX_USE_SMP)
if (EXPERIMENTAL_USE_DOT)
    add_definitions( -DEXPERIMENTAL_USE_DOT )
    message( "-- Experimental 'dot' extension")
endif (EXPERIMENTAL_USE_DOT)
if (EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT)
    add_definitions( -DEXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT )
    message( "-- Experimental F16 extension")
endif (EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT)
if (EXPERIMENTAL_USE_VENUM)
    add_definitions( -DEXPERIMENTAL_USE_VENUM )
    message( "-- Experimental raspberrypi 3B+ neon extension")
endif (EXPERIMENTAL_USE_VENUM)
message( "" )


if (BUILD_X64)
    add_definitions( -DARCH_64 )
else (BUILD_X64)
    add_definitions( -DARCH_32 )
endif (BUILD_X64)

if (CMAKE_BUILD_TYPE EQUAL Debug)
    add_definitions( -DOPENVX_DEBUGGING )
endif (CMAKE_BUILD_TYPE EQUAL Debug)

if (OPENVX_USE_XML)
    find_package(LibXml2)
    if (LIBXML2_FOUND)
        message( "-- Found the required XML library")
        message( "--    XML includes = ${LIBXML2_INCLUDE_DIR}")
        message( "--    XML library = ${LIBXML2_LIBRARIES}")
        message( "--    XML link directories = ${LIBXML2_LINK_DIRECTORIES}")
    else (LIBXML2_FOUND)
        message( FATAL_ERROR "Could not find the XML library")
    endif (LIBXML2_FOUND)
endif (OPENVX_USE_XML)

FIND_NUM_PROCESSORS()
if ( ${PROCESSOR_COUNT} LESS "1" )
        message(FATAL_ERROR "Num processors found is " ${PROCESSOR_COUNT})
endif ( ${PROCESSOR_COUNT} LESS "1" )
add_definitions( -DTARGET_NUM_CORES=${PROCESSOR_COUNT} )

# Framework IDE Folders Names
set_property( GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "" )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )
set( DEBUG_FOLDER Debug )
set( EXAMPLES_FOLDER Examples )
set( HELPER_FOLDER Helper )
set( KERNELS_FOLDER Kernels )
set( LIBRARIES_FOLDER Libraries )
set( LIBRARIES_DEBUG_FOLDER ${LIBRARIES_FOLDER}/Debug )
set( LIBRARIES_EXTRAS_FOLDER ${LIBRARIES_FOLDER}/Extras )
set( SAMPLE_FOLDER Sample )
set( SAMPLE_TARGETS_FOLDER ${SAMPLE_FOLDER}/Targets )
set( SAMPLE_CPP_FOLDER SampleCPP )
set( TOOLS_FOLDER Tools )


add_subdirectory( include )
add_subdirectory( debug )
add_subdirectory( examples )
add_subdirectory( helper )
add_subdirectory( kernels )
add_subdirectory( libraries )
add_subdirectory( sample )
add_subdirectory( tools )
add_subdirectory( utils )
