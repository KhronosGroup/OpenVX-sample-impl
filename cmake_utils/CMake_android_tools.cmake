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


set( TOOL_OS_SUFFIX "" )
if (BUILD_X64)
    set( ANDROID_TOOLCHAIN_DIR_NAME "android-toolchain-x86_64" )
	set( ANDROID_TOOLS_PREFIX "x86_64" )
else (BUILD_X64)
    set( ANDROID_TOOLCHAIN_DIR_NAME "android-toolchain" )
	set( ANDROID_TOOLS_PREFIX "i686" )
endif (BUILD_X64)

if ((DEFINED ENV{ANDROID_NDK_TOOLCHAIN_ROOT}) AND (IS_DIRECTORY $ENV{ANDROID_NDK_TOOLCHAIN_ROOT}))
    set( ANDROID_NDK_TOOLCHAIN_ROOT $ENV{ANDROID_NDK_TOOLCHAIN_ROOT} )
else ()
	execute_process(COMMAND find "/usr/local/" -name ${ANDROID_TOOLCHAIN_DIR_NAME} -type d OUTPUT_VARIABLE ANDROID_NDK_TOOLCHAIN_ROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
endif ((DEFINED ENV{ANDROID_NDK_TOOLCHAIN_ROOT}) AND (IS_DIRECTORY $ENV{ANDROID_NDK_TOOLCHAIN_ROOT}))
if (NOT IS_DIRECTORY ${ANDROID_NDK_TOOLCHAIN_ROOT})
    message(FATAL_ERROR "Can't find Android ToolChain Root ${ANDROID_NDK_TOOLCHAIN_ROOT}")
endif (NOT IS_DIRECTORY ${ANDROID_NDK_TOOLCHAIN_ROOT})

message("ANDROID_NDK_TOOLCHAIN_ROOT = " ${ANDROID_NDK_TOOLCHAIN_ROOT})

set(CMAKE_FIND_ROOT_PATH  ${ANDROID_NDK_TOOLCHAIN_ROOT} )

# specify gcc version
execute_process(COMMAND "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-gcc${TOOL_OS_SUFFIX}" --version
                OUTPUT_VARIABLE GCC_VERSION_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE )
STRING( REGEX MATCH "[0-9](\\.[0-9])+" GCC_VERSION "${GCC_VERSION_OUTPUT}" )

# specify the cross compiler
set( CMAKE_C_COMPILER   "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-gcc${TOOL_OS_SUFFIX}"     CACHE PATH "gcc" FORCE )
set( CMAKE_CXX_COMPILER "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-g++${TOOL_OS_SUFFIX}"     CACHE PATH "g++" FORCE )
set( CMAKE_AR           "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-ar${TOOL_OS_SUFFIX}"      CACHE PATH "archive" FORCE )
set( CMAKE_LINKER       "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-ld${TOOL_OS_SUFFIX}"      CACHE PATH "linker" FORCE )
set( CMAKE_NM           "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-nm${TOOL_OS_SUFFIX}"      CACHE PATH "nm" FORCE )
set( CMAKE_OBJCOPY      "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-objcopy${TOOL_OS_SUFFIX}" CACHE PATH "objcopy" FORCE )
set( CMAKE_OBJDUMP      "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-objdump${TOOL_OS_SUFFIX}" CACHE PATH "objdump" FORCE )
set( CMAKE_STRIP        "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-strip${TOOL_OS_SUFFIX}"   CACHE PATH "strip" FORCE )
set( CMAKE_RANLIB       "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLS_PREFIX}-linux-android-ranlib${TOOL_OS_SUFFIX}"  CACHE PATH "ranlib" FORCE )

set(CMAKE_FIND_ROOT_PATH  "${ANDROID_NDK_TOOLCHAIN_ROOT}/bin" "${ANDROID_NDK_TOOLCHAIN_ROOT}/${ANDROID_TOOLS_PREFIX}-linux-android" "${ANDROID_NDK_TOOLCHAIN_ROOT}/sysoot" )

###########################################################

message(STATUS "** ** ** Enable Languages ** ** **")

enable_language( C )
enable_language( CXX )

if(BUILD_X64)
  set(ARCH_BIT -m64 )
else()
  if (TARGET_CPU STREQUAL "Atom")
    # architecture will be according to ATOM
    set(ARCH_BIT -m32 )
  else ()
    # need to force a more modern architecture than the degault m32 (i386).
    set(ARCH_BIT "-m32 -march=core2" )
  endif (TARGET_CPU STREQUAL "Atom")
endif()

# Compiler switches that CANNOT be modified during makefile generation
set (ADD_COMMON_C_FLAGS         "${ARCH_BIT} -fPIC" )

set (ADD_C_FLAGS                "${ADD_COMMON_C_FLAGS} -std=gnu99" )
set (ADD_C_FLAGS_DEBUG          "-O0 -ggdb3 -D _DEBUG" )
set (ADD_C_FLAGS_RELEASE        "-O2 -ggdb2 -U _DEBUG")
set (ADD_C_FLAGS_RELWITHDEBINFO "-O2 -ggdb3 -U _DEBUG")

set (ADD_CXX_FLAGS              "${ADD_COMMON_C_FLAGS}" )

# Linker switches

set (INIT_LINKER_FLAGS          "-Wl,--enable-new-dtags" ) # --enable-new-dtags sets RUNPATH to the same value as RPATH
# embed RPATH and RUNPATH to the binaries that assumes that everything is installed in the same directory
#
# Description:
#   RPATH is used to locate dynamically load shared libraries/objects (DLLs) for the non-standard OS
#   locations without need of relinking DLLs during installation. The algorithm is the following:
#
#   1. If RPATH is present in the EXE/DLL and RUNPATH is NOT present, search through it.
#   2. If LD_LIBRARY_PATH env variable is present, search through it
#   3. If RUNPATH is present in the EXE/DLL, search through it
#   4. Search through locations, configured by system admin and cached in /etc/ld.so.cache
#   5. Search through /lib and /usr/lib
#
#   RUNPATH influences only the immediate dependencies, while RPATH influences the whole subtree of dependencies
#   RPATH is concidered deprecated in favor of RUNPATH, but RUNPATH does not supported by some Linux systems.
#   If RUNPATH is not supported, system loader may report error - remove "-Wl,--enable-new-dtags" above to
#   disable RUNPATH generation.
#
#   If RPATH or RUNPATH contains string $ORIGIN it is substituted by the full path to the containing EXE/DLL.
#   Security issue 1: if EXE/DLL is marked as set-uid or set-gid, $ORIGIN is ignored.
#   Security issue 2: if RPATH/RUNPATH references relative subdirs, intruder may fool it by using softlinks
#
SET(CMAKE_BUILD_WITH_INSTALL_RPATH    TRUE )   # build rpath as if already installed
SET(CMAKE_INSTALL_RPATH               "$ORIGIN::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::" ) # the rpath to use - search through installation dir only
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)   # do not use static link paths as rpath

# C switches
set( CMAKE_C_FLAGS                          "${CMAKE_C_FLAGS}                         ${ADD_C_FLAGS}")
set( CMAKE_C_FLAGS_DEBUG                    "${CMAKE_C_FLAGS_DEBUG}                   ${ADD_C_FLAGS_DEBUG}")
set( CMAKE_C_FLAGS_RELEASE                  "${CMAKE_C_FLAGS_RELEASE}                 ${ADD_C_FLAGS_RELEASE}")
set( CMAKE_C_FLAGS_RELWITHDEBINFO           "${CMAKE_C_FLAGS_RELWITHDEBINFO}          ${ADD_C_FLAGS_RELWITHDEBINFO}")

# C++ switches
set( CMAKE_CXX_FLAGS                        "${CMAKE_CXX_FLAGS}                       ${ADD_CXX_FLAGS}")
set( CMAKE_CXX_FLAGS_DEBUG                  "${CMAKE_CXX_FLAGS_DEBUG}                 ${ADD_C_FLAGS_DEBUG}")
set( CMAKE_CXX_FLAGS_RELEASE                "${CMAKE_CXX_FLAGS_RELEASE}               ${ADD_C_FLAGS_RELEASE}")
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO         "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}        ${ADD_C_FLAGS_RELWITHDEBINFO}")

# Linker switches - EXE
set( CMAKE_EXE_LINKER_FLAGS                 "${INIT_LINKER_FLAGS}")

# Linker switches - DLL
set( CMAKE_SHARED_LINKER_FLAGS              "${INIT_LINKER_FLAGS}                     ${ADD_CMAKE_EXE_LINKER_FLAGS}")

message(STATUS "\n\n** ** ** COMPILER Definitions ** ** **")
message(STATUS "CMAKE_C_COMPILER        = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_C_FLAGS           = ${CMAKE_C_FLAGS}")
message(STATUS "")
message(STATUS "CMAKE_CXX_COMPILER      = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_FLAGS         = ${CMAKE_CXX_FLAGS}")
message(STATUS "")
message(STATUS "CMAKE_EXE_LINKER_FLAGS  = ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "")
message(STATUS "CMAKE_BUILD_TOOL        = ${CMAKE_BUILD_TOOL}")

