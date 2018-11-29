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


# Compiler switches that CANNOT be modified during makefile generation
set (ADD_C_FLAGS         "/Oi -D WINDOWS_ENABLE_CPLUSPLUS /GS")
set (ADD_C_FLAGS_DEBUG   "-D _DEBUG /RTC1 /MTd")  #/MTd /Gm
set (ADD_C_FLAGS_RELEASE "/Zi /Gy -D NDEBUG /MT")# /Ob0") #/GL") #MT

# Compiler switches that CAN be modified during makefile generation and configuration-independent
add_definitions( -DWIN32 -D_WIN32 )

if (BUILD_X64)
    add_definitions(-DARCH_64)
else (BUILD_X64)
    add_definitions(-DARCH_32)
endif (BUILD_X64)

# Linker switches
if (BUILD_X64)
    set (INIT_LINKER_FLAGS        "/MACHINE:X64 /OPT:REF /INCREMENTAL:NO /NXCOMPAT")
else (BUILD_X64)
    set (INIT_LINKER_FLAGS        "/MACHINE:X86 /OPT:REF /INCREMENTAL:NO /NXCOMPAT /SAFESEH")
endif (BUILD_X64)

set (ADD_LINKER_FLAGS_DEBUG "/DEBUG /NODEFAULTLIB:LIBCMT /NODEFAULTLIB:LIBCPMT")
set (ADD_LINKER_FLAGS_RELEASE "/OPT:REF /OPT:ICF /NODEFAULTLIB:LIBCMTD /NODEFAULTLIB:LIBCPMTD")


# setup
enable_language( C )
enable_language( CXX )

set( OPENVX_BUILDING_EXPORT_DEF -DVX_API_ENTRY=__declspec\(dllexport\) )

add_definitions( -D_CRT_SECURE_NO_WARNINGS=1 )

# remove CRT DLL option from default C/C++ switches
string( REPLACE /MDd "" CMAKE_C_FLAGS_DEBUG       ${CMAKE_C_FLAGS_DEBUG} )
string( REPLACE /MD  "" CMAKE_C_FLAGS_RELEASE     ${CMAKE_C_FLAGS_RELEASE} )
string( REPLACE /MDd "" CMAKE_CXX_FLAGS_DEBUG     ${CMAKE_CXX_FLAGS_DEBUG} )
string( REPLACE /MD  "" CMAKE_CXX_FLAGS_RELEASE   ${CMAKE_CXX_FLAGS_RELEASE} )

# remove /INCREMENTAL:YES option from DEBUG Linker switches
string( REPLACE /INCREMENTAL:YES "" CMAKE_EXE_LINKER_FLAGS_DEBUG    ${CMAKE_EXE_LINKER_FLAGS_DEBUG} )
string( REPLACE /INCREMENTAL:YES "" CMAKE_SHARED_LINKER_FLAGS_DEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} )

# C switches
set( CMAKE_C_FLAGS         "${CMAKE_C_FLAGS}         ${ADD_C_FLAGS}")
set( CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}   ${ADD_C_FLAGS_DEBUG}")
set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${ADD_C_FLAGS_RELEASE}")

# C++ switches
set( CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS}         ${ADD_C_FLAGS}")
set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   ${ADD_C_FLAGS_DEBUG}")
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${ADD_C_FLAGS_RELEASE}")

# Linker switches - EXE
set( CMAKE_EXE_LINKER_FLAGS           ${INIT_LINKER_FLAGS})
set( CMAKE_EXE_LINKER_FLAGS_DEBUG     "${CMAKE_EXE_LINKER_FLAGS_DEBUG}   ${ADD_LINKER_FLAGS_DEBUG}")
set( CMAKE_EXE_LINKER_FLAGS_RELEASE   "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${ADD_LINKER_FLAGS_RELEASE}")

# Linker switches - DLL
set( CMAKE_SHARED_LINKER_FLAGS          ${INIT_LINKER_FLAGS})
set( CMAKE_SHARED_LINKER_FLAGS_DEBUG   "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}   ${ADD_LINKER_FLAGS_DEBUG}")
set( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${ADD_LINKER_FLAGS_RELEASE}")

