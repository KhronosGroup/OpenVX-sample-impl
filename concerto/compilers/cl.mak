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

# check for the supported CPU types for this compiler 
ifeq ($(filter $(TARGET_FAMILY),X86 x86_64 X64),)
$(error TARGET_FAMILY $(TARGET_FAMILY) is not supported by this compiler)
endif

# check for the support OS types for this compiler
ifeq ($(filter $(TARGET_OS),Windows_NT),)
$(error TARGET_OS $(TARGET_OS) is not supported by this compiler)
endif

CC = CL
CP = CL
AS = $(TARGET_CPU)ASM
AR = LIB
LD = LINK

ifdef LOGFILE
LOGGING=>$(LOGFILE)
endif

ifeq ($(strip $(TARGETTYPE)),library)
	BIN_PRE=
	BIN_EXT=.lib
else ifeq ($(strip $(TARGETTYPE)),dsmo)
	BIN_PRE=
	BIN_EXT=.dll
else ifeq ($(strip $(TARGETTYPE)),exe)
	BIN_PRE=
	BIN_EXT=.exe
endif

$(_MODULE)_SYS_SHARED_LIBS += user32
$(_MODULE)_OUT  := $(BIN_PRE)$($(_MODULE)_TARGET)$(BIN_EXT)
$(_MODULE)_BIN  := $($(_MODULE)_TDIR)/$($(_MODULE)_OUT)
$(_MODULE)_OBJS := $(ASSEMBLY:%.S=$($(_MODULE)_ODIR)/%.obj) $(CPPSOURCES:%.cpp=$($(_MODULE)_ODIR)/%.obj) $(CSOURCES:%.c=$($(_MODULE)_ODIR)/%.obj)
# Redefine the local static libs and shared libs with REAL paths and pre/post-fixes
$(_MODULE)_STATIC_LIBS := $(foreach lib,$(STATIC_LIBS),$($(_MODULE)_TDIR)/$(lib).lib) 
$(_MODULE)_SHARED_LIBS := $(foreach lib,$(SHARED_LIBS),$($(_MODULE)_TDIR)/$(lib).dll) 
ifeq ($(BUILD_MULTI_PROJECT),1)
$(_MODULE)_STATIC_LIBS += $(foreach lib,$(SYS_STATIC_LIBS),$($(_MODULE)_TDIR)/$(lib).lib)
$(_MODULE)_SHARED_LIBS += $(foreach lib,$(SYS_SHARED_LIBS),$($(_MODULE)_TDIR)/$(lib).dll)
$(_MODULE)_PLATFORM_LIBS := $(foreach lib,$(PLATFORM_LIBS),$($(_MODULE)_TDIR)/$(lib).dll)
endif
$(_MODULE)_PDB  := $($(_MODULE)_ODIR)/$(TARGET).pdb

$(_MODULE)_COPT+=/EHsc /W3
ifeq ($(TARGET_CPU),X64)
$(_MODULE)_COPT+=/Wp64
endif
ifeq ($(TARGET_BUILD),debug)
$(_MODULE)_COPT+=/Od /MDd /Gm /Zi /RTC1
else ifneq ($(filter $(TARGET_BUILD),release production),)
$(_MODULE)_COPT+=/Ox /MD
endif

$(_MODULE)_INCLUDES := $(foreach inc,$(call PATH_CONV,$($(_MODULE)_IDIRS)),/I$(inc))
$(_MODULE)_DEFINES  := $(foreach def,$($(_MODULE)_DEFS),/D$(def))
$(_MODULE)_LIBRARIES:= $(foreach ldir,$(call PATH_CONV,$($(_MODULE)_LDIRS)),/LIBPATH:$(ldir)) $(foreach lib,$(STATIC_LIBS),$(lib).lib) $(foreach lib,$(SHARED_LIBS),$(lib).lib) $(foreach lib,$(SYS_STATIC_LIBS),$(lib).lib) $(foreach lib,$(SYS_SHARED_LIBS),$(lib).lib) $(foreach lib,$(PLATFORM_LIBS),$(lib).lib)
$(_MODULE)_ARFLAGS  := /nologo /MACHINE:$(TARGET_FAMILY)
$(_MODULE)_AFLAGS   := $($(_MODULE)_INCLUDES)
$(_MODULE)_LDFLAGS  := /nologo /MACHINE:$(TARGET_FAMILY)
$(_MODULE)_CFLAGS   := /c /nologo $($(_MODULE)_INCLUDES) $($(_MODULE)_DEFINES) $($(_MODULE)_COPT) $(CFLAGS)

ifdef ENTRY
$(_MODULE)_ENTRY := $(ENTRY)
$(_MODULE)_LDFLAGS += /ENTRY:$($(_MODULE)_ENTRY) /SUBSYSTEM:WINDOWS
endif

ifdef DEFFILE
$(_MODULE)_DEF:=/DEF:$(call PATH_CONV,$($(_MODULE)_SDIR)/$(DEFFILE))
$(_MODULE)_DEPS:=$($(_MODULE)_SDIR)/$(DEFFILE)
else
$(_MODULE)_DEF:=
$(_MODULE)_DEPS:=
endif

ifeq ($(TARGET_BUILD),debug)
$(_MODULE)_LDFLAGS += /DEBUG
endif

###################################################
# COMMANDS
###################################################

$(_MODULE)_LINK_LIB  := $(AR) $($(_MODULE)_ARFLAGS) /OUT:$(call PATH_CONV,$($(_MODULE)_BIN)) $(call PATH_CONV,$($(_MODULE)_OBJS)) $(call PATH_CONV,$($(_MODULE)_LIBS))
$(_MODULE)_LINK_EXE  := $(LD) $($(_MODULE)_LDFLAGS) $(call PATH_CONV,$($(_MODULE)_OBJS)) $($(_MODULE)_LIBRARIES) /OUT:$(call PATH_CONV,$($(_MODULE)_BIN))
$(_MODULE)_LINK_DSO  := $(LD) $($(_MODULE)_LDFLAGS) $(call PATH_CONV,$($(_MODULE)_OBJS)) $($(_MODULE)_LIBRARIES) /DLL $($(_MODULE)_DEF) /OUT:$(call PATH_CONV,$($(_MODULE)_BIN))

###################################################
# MACROS FOR COMPILING
###################################################

define $(_MODULE)_BUILD
build:: $($(_MODULE)_BIN)
endef

define $(_MODULE)_COMPILE_TOOLS
$($(_MODULE)_ODIR)/%.obj: $($(_MODULE)_SDIR)/%.c
#	@echo [PURE] Compiling MSFT C $$(notdir $$<)
	$(Q)$(CC) $($(_MODULE)_CFLAGS) $$(call PATH_CONV,$$<) /Fo$$(call PATH_CONV,$$@) /Fd$$(call PATH_CONV,$($(_MODULE)_ODIR)/$$(notdir $$(basename $$<)).pdb) $(LOGGING)

$($(_MODULE)_ODIR)/%.obj: $($(_MODULE)_SDIR)/%.cpp
#	@echo [PURE] Compiling MSFT C++ $$(notdir $$<)
	$(Q)$(CP) $($(_MODULE)_CFLAGS) $$(call PATH_CONV,$$<) /Fo$$(call PATH_CONV,$$@) /Fd$$(call PATH_CONV,$($(_MODULE)_ODIR)/$$(notdir $$(basename $$<)).pdb) $(LOGGING)

$($(_MODULE)_ODIR)/%.obj: $($(_MODULE)_SDIR)/%.S
#	@echo [PURE] Assembling NASM $$(notdir $$<)
	$(Q)$(AS) $($(_MODULE)_AFLAGS) $$(call PATH_CONV,$$<) /Fo$$(call PATH_CONV,$$@) /Fd$$(call PATH_CONV,$($(_MODULE)_ODIR)/$$(notdir $$(basename $$<)).pdb)  $(LOGGING)
endef
