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
ifeq ($(filter $(TARGET_FAMILY),ARM),)
$(error TARGET_FAMILY $(TARGET_FAMILY) is not supported by this compiler)
endif 

# check for the support OS types for this compiler
ifeq ($(filter $(TARGET_OS),LINUX Windows_NT),)
$(error TARGET_OS $(TARGET_OS) is not supported by this compiler)
endif

CC = armcc
CP = armcc
AS = armasm
AR = armar
LD = armcc

ifdef LOGFILE
LOGGING:=&>$(LOGFILE)
else
LOGGING:=
endif

ifeq ($(TARGET_OS),Windows_NT)
DSO_EXT := .dll
else
DSO_EXT := .so
endif

ifeq ($(strip $($(_MODULE)_TYPE)),library)
	BIN_PRE=lib
	BIN_EXT=.a
else ifeq ($(strip $($(_MODULE)_TYPE)),dsmo)
	BIN_PRE=lib
	BIN_EXT=$(DSO_EXT)
else
	BIN_PRE=
	BIN_EXT=
endif

$(_MODULE)_OUT  := $(BIN_PRE)$($(_MODULE)_TARGET)$(BIN_EXT)
$(_MODULE)_BIN  := $($(_MODULE)_TDIR)/$($(_MODULE)_OUT)
$(_MODULE)_OBJS := $(ASSEMBLY:%.S=$($(_MODULE)_ODIR)/%.o) $(CPPSOURCES:%.cpp=$($(_MODULE)_ODIR)/%.o) $(CSOURCES:%.c=$($(_MODULE)_ODIR)/%.o)
# Redefine the local static libs and shared libs with REAL paths and pre/post-fixes
$(_MODULE)_STATIC_LIBS := $(foreach lib,$(STATIC_LIBS),$($(_MODULE)_TDIR)/lib$(lib).a)
$(_MODULE)_SHARED_LIBS := $(foreach lib,$(SHARED_LIBS),$($(_MODULE)_TDIR)/lib$(lib)$(DSO_EXT))
ifeq ($(BUILD_MULTI_PROJECT),1)
$(_MODULE)_STATIC_LIBS += $(foreach lib,$(SYS_STATIC_LIBS),$($(_MODULE)_TDIR)/lib$(lib).a)
$(_MODULE)_SHARED_LIBS += $(foreach lib,$(SYS_SHARED_LIBS),$($(_MODULE)_TDIR)/lib$(lib)$(DSO_EXT))
$(_MODULE)_PLATFORM_LIBS := $(foreach lib,$(PLATFORM_LIBS),$($(_MODULE)_TDIR)/lib$(lib)$(DSO_EXT))
endif
$(_MODULE)_DEP_HEADERS := $(foreach inc,$($(_MODULE)_HEADERS),$($(_MODULE)_SDIR)/$(inc).h)

$(_MODULE)_COPT := $(CFLAGS)
$(_MODULE)_LOPT := $(LDFLAGS)
$(_MODULE)_COPT += -Wall -fms-extensions -Wno-write-strings

ifeq ($(TARGET_BUILD),debug)
$(_MODULE)_COPT += -g
else ifeq ($(TARGET_BUILD),release)
$(_MODULE)_COPT += -O3 -Otime
$(_MODULE)_LOPT += --no_debug
endif
ifeq ($(TARGET_ENDIAN),LITTLE)
$(_MODULE)_COPT += -mlittle-endian
else
$(_MODULE)_COPT += -mbig-endian
endif

ifeq ($(TARGET_OS),LINUX)
$(_MODULE)_COPT += --arm_linux_paths --arm_linux_config_file=$($(_MODULE)_CONFIG) --apcs=/fpic -W
$(_MODULE)_LOPT += --arm_linux --arm_linux_config_file=$($(_MODULE)_CONFIG) --apcs=/fpic
endif

ifeq ($(HOST_CPU),$(TARGET_CPU))
else ifeq ($(TARGET_CPU),M3)
else ifeq ($(TARGET_CPU),M4)
else ifeq ($(TARGET_CPU),A8)
else ifeq ($(TARGET_CPU),A9)
else ifeq ($(TARGET_CPU),A15)
endif


$(_MODULE)_MAP      := $($(_MODULE)_BIN).map
$(_MODULE)_INCLUDES := $(foreach inc,$($(_MODULE)_IDIRS),-I$(inc))
$(_MODULE)_DEFINES  := $(foreach def,$($(_MODULE)_DEFS),-D$(def))
$(_MODULE)_LIBRARIES:= $(foreach ldir,$($(_MODULE)_LDIRS),-L--userlibpath=$(ldir)) \
                       $(foreach lib,$(STATIC_LIBS),-Llib$(lib).a) \
                       $(foreach lib,$(SYS_STATIC_LIBS),-Llib$(lib).a) \
                       $(foreach lib,$(SHARED_LIBS),-Llib$(lib)$(DSO_EXT)) \
                       $(foreach lib,$(SYS_SHARED_LIBS),-Llib$(lib)$(DSO_EXT))\
					   $(foreach lib,$(PLATFORM_LIBS),-Llib$(lib)$(DSO_EXT))
$(_MODULE)_AFLAGS   := $($(_MODULE)_INCLUDES)
$(_MODULE)_LDFLAGS  += $($(_MODULE)_LOPT)
$(_MODULE)_CFLAGS   := -c $($(_MODULE)_INCLUDES) $($(_MODULE)_DEFINES) $($(_MODULE)_COPT) $(CFLAGS)

###################################################
# COMMANDS
###################################################

$(_MODULE)_LN_DSO     := $(LINK) $($(_MODULE)_BIN).1.0 $($(_MODULE)_BIN)
$(_MODULE)_LN_INST_DSO:= $(LINK) $($(_MODULE)_INSTALL_LIB)/$($(_MODULE)_OUT).1.0 $($(_MODULE)_INSTALL_LIB)/$($(_MODULE)_OUT)
$(_MODULE)_LINK_LIB   := $(AR) -rscu $($(_MODULE)_BIN) $($(_MODULE)_OBJS) #$($(_MODULE)_STATIC_LIBS)
$(_MODULE)_LINK_DSO   := $(LD) $($(_MODULE)_LDFLAGS) --shared $($(_MODULE)_OBJS) $($(_MODULE)_LIBRARIES) -o $($(_MODULE)_BIN).1.0
$(_MODULE)_LINK_EXE   := $(LD) $($(_MODULE)_LDFLAGS) $($(_MODULE)_OBJS) $($(_MODULE)_LIBRARIES) -o $($(_MODULE)_BIN)

###################################################
# MACROS FOR COMPILING
###################################################

define $(_MODULE)_BUILD
build:: $($(_MODULE)_BIN)
endef

define $(_MODULE)_COMPILE_TOOLS

$($(_MODULE)_CONFIG): $($(_MODULE)_TDIR)/.gitignore
	@echo [RVCT] Building Configuration File $$(notdir $$<)
	$(Q)$(CC) --arm_linux_configure --arm_linux_config_file=$($(_MODULE)_CONFIG) --configure_gcc=/usr/bin/$(CROSS_COMPILE)gcc

$(ODIR)/%.o: $(SDIR)/%.c $($(_MODULE)_CONFIG)
	@echo [RVCT] Compiling C90 $$(notdir $$<)
	$(Q)$(CC) --c90 $($(_MODULE)_CFLAGS) $$< -o $$@ $(LOGGING)

$(ODIR)/%.o: $(SDIR)/%.cpp $($(_MODULE)_CONFIG)
	@echo [RVCT] Compiling C++ $$(notdir $$<)
	$(Q)$(CP) --cpp $($(_MODULE)_CFLAGS) $$< -o $$@ $(LOGGING)

$(ODIR)/%.o: $(SDIR)/%.s $($(_MODULE)_CONFIG)
	@echo [RVCT] Assembling $$(notdir $$<)
	$(Q)$(AS) $($(_MODULE)_AFLAGS) $$< -o $$@ $(LOGGING)
endef
