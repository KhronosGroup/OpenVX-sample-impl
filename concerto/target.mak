# 

# Copyright (c) 2012-2017 The Khronos Group Inc.
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


# @author Erik Rainey
# @url http://github.com/emrainey/Concerto

SYSIDIRS := $(HOST_ROOT)/api-docs/include $(HOST_ROOT)/include
SYSLDIRS :=
SYSDEFS  := OPENVX_BUILDING 
#SYSDEFS  += OPENVX_USE_SMP
#SYSDEFS  += OPENVX_USE_TILING
SYSDEFS  += OPENVX_USE_IX
SYSDEFS  += OPENVX_USE_NN
SYSDEFS  += OPENVX_USE_NN_16
SYSDEFS  += OPENVX_CONFORMANCE_VISION
SYSDEFS  += OPENVX_CONFORMANCE_NEURAL_NETWORKS
SYSDEFS  += OPENVX_CONFORMANCE_VISION_NNEF_IMPORT
SYSDEFS  += OPENVX_USE_ENHANCED_VISION

ifeq ($(TARGET_BUILD),debug)
SYSDEFS += OPENVX_DEBUGGING
endif

ifeq ($(TARGET_PLATFORM),PC)
    ifeq ($(TARGET_OS),LINUX)
        INSTALL_LIB := /usr/lib
        INSTALL_BIN := /usr/bin
        INSTALL_INC := /usr/include
        SYSIDIRS += /usr/include
        SYSLDIRS += /usr/lib
        SYSDEFS += _XOPEN_SOURCE=700 _GNU_SOURCE=1
        SYSDEFS += EXPERIMENTAL_USE_DOT        # should be "libxml-2.0" on Ubuntu
        ifneq ($(XML2_PKG),)
            XML2_LIBS := $(subst -l,,$(shell pkg-config --libs-only-l $(XML2_PKG)))
            XML2_INCS := $(subst -I,,$(shell pkg-config --cflags $(XML2_PKG)))
            SYSDEFS += OPENVX_USE_XML
            SYSIDIRS += $(XML2_INCS)
        endif
        ifneq ($(SDL_PKG),)
            SDL_LIBS := $(subst -l,,$(shell pkg-config --libs-only-l $(SDL_PKG)))
            SDL_INCS := $(subst -I,,$(shell pkg-config --cflags-only-I $(SDL_PKG)))
            SYSDEFS += $(subst -D,,$(shell pkg-config --cflags-only-other $(SDL_PKG))) OPENVX_USE_SDL
            SYSIDIRS += $(SDL_INCS)
        endif
    else ifeq ($(TARGET_OS),DARWIN)
        INSTALL_LIB := /opt/local/lib
        INSTALL_BIN := /opt/local/bin
        INSTALL_INC := /opt/local/include
        SYSDEFS += _XOPEN_SOURCE=700 _BSD_SOURCE=1 _GNU_SOURCE=1
        SYSDEFS += EXPERIMENTAL_USE_DOT
        XML2_PATH := $(dir $(shell brew list libxml2 | grep -m 1 include))
        XML2_BREWROOT := $(patsubst %/include/libxml2/libxml/,%,$(XML2_PATH))
        ifneq ($(XML2_BREWROOT),)
            XML2_LIBS := xml2
            SYSDEFS += OPENVX_USE_XML
            SYSIDIRS += $(XML2_BREWROOT)/include/libxml2
            SYSLDIRS += $(XML2_BREWROOT)/lib
        endif
    else ifeq ($(TARGET_OS),CYGWIN)
        INSTALL_LIB := /usr/lib
        INSTALL_BIN := /usr/bin
        INSTALL_INC := /usr/include
        SYSDEFS += _XOPEN_SOURCE=700 _BSD_SOURCE=1 _GNU_SOURCE=1 WINVER=0x501
        ifneq (,$(findstring OPENVX_BUILDING,$(SYSDEFS)))
            SYSDEFS += 'VX_API_ENTRY=__attribute__((dllexport))'
        else
            SYSDEFS += 'VX_API_ENTRY=__attribute__((dllimport))'
        endif
    else ifeq ($(TARGET_OS),Windows_NT)
        INSTALL_LIB := "${windir}\\system32"
        INSTALL_BIN := "${windir}\\system32"
        INSTALL_INC :=
        SYSIDIRS += $(HOST_ROOT)/include/windows
        SYSDEFS += WIN32_LEAN_AND_MEAN WIN32 _WIN32 _CRT_SECURE_NO_DEPRECATE WINVER=0x0501 _WIN32_WINNT=0x0501 NTDDI_VERSION=0x05010100 # NTDDI_VERSION > NTDDI_WINXP required for rtlStackBackTrace API
        ifneq (,$(findstring OPENVX_BUILDING,$(SYSDEFS)))
            SYSDEFS += VX_API_ENTRY=__declspec(dllexport)
        else
            SYSDEFS += VX_API_ENTRY=__declspec(dllimport)
        endif
    endif
endif

