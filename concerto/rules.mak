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

# Basic definitions for parsing and tokenizing
EMPTY:=
SPACE:=$(EMPTY) $(EMPTY)
COMMA:=$(EMPTY),$(EMPTY)
TOKENS=$(subst :,$(SPACE),$(1))
# Macros for text manipulation
lowercase = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))
uppercase = $(subst a,A,$(subst b,B,$(subst c,C,$(subst d,D,$(subst e,E,$(subst f,F,$(subst g,G,$(subst h,H,$(subst i,I,$(subst j,J,$(subst k,K,$(subst l,L,$(subst m,M,$(subst n,N,$(subst o,O,$(subst p,P,$(subst q,Q,$(subst r,R,$(subst s,S,$(subst t,T,$(subst u,U,$(subst v,V,$(subst w,W,$(subst x,X,$(subst y,Y,$(subst z,Z,$1))))))))))))))))))))))))))
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Define all but don't do anything with it yet.
.PHONY: all
all::

# Remove the implicit rules for compiling.
.SUFFIXES:

# Define our pathing and default variable values
HOST_ROOT ?= $(abspath .)
BUILD_FOLDER ?= concerto
CONCERTO_ROOT ?= $(HOST_ROOT)/$(BUILD_FOLDER)
BUILD_OUTPUT ?= out
BUILD_TARGET ?= $(CONCERTO_ROOT)/target.mak
BUILD_PLATFORM ?= $(CONCERTO_ROOT)/platform.mak
BUILD_PROJECT ?= $(CONCERTO_ROOT)/project.mak
DIRECTORIES ?= source
ifeq ($(NO_OPTIMIZE),1)
TARGET_BUILD?=debug
else ifeq ($(FINAL_BUILD),1)
TARGET_BUILD?=production
else
TARGET_BUILD?=release
endif

# Define the prelude and finale files so that SUBMAKEFILEs know what they are
# And if the users go and make -f concerto.mak then it will not work right.
PRELUDE := $(CONCERTO_ROOT)/prelude.mak
FINALE  := $(CONCERTO_ROOT)/finale.mak
SUBMAKEFILE := concerto.mak

# Allows the commands to be printed out when invoked
ifeq ($(BUILD_DEBUG),1)
SHOW_COMMANDS:=1
SHOW_MAKEDEBUG:=1
endif

ifeq ($(SHOW_COMMANDS),1)
Q:=
else
Q:=@
endif

include $(CONCERTO_ROOT)/os.mak
include $(CONCERTO_ROOT)/machine.mak
include $(CONCERTO_ROOT)/shell.mak
include $(CONCERTO_ROOT)/scm.mak

# Check for COMBOS, if none existed make a single COMBO
TARGET_COMBOS ?= PC:$(HOST_OS):$(HOST_CPU):0:$(TARGET_BUILD):$(HOST_COMPILER)

# Find all the Makfiles in the subfolders, these will be pulled in to make
TARGET_MAKEFILES := $(filter %/$(SUBMAKEFILE),$(sort $(foreach d,$(DIRECTORIES),$(strip $(call rwildcard,$(d)/,*.mak)))))

# These variables will be appended by each new submakefile included in the combo
MODULES:=
CONCERTO_TARGETS :=
TESTABLE_MODULES :=

# Define a macro to make the output target path
MAKE_OUT = $(1)/$(BUILD_OUTPUT)/$(TARGET_OS)/$(TARGET_CPU)/$(TARGET_BUILD)

# Define a macro to remove a combo from the combos list if it matches a value
FILTER_COMBO = $(foreach combo,$(TARGET_COMBOS),$(if $(filter $(1),$(subst :, ,$(combo))),$(combo)))
FILTER_OUT_COMBO = $(foreach combo,$(TARGET_COMBOS),$(if $(filter $(1),$(subst :, ,$(combo))), ,$(combo))) 

# Macro to include the combo rules
define CONCERTO_BUILD
include $(CONCERTO_ROOT)/combo.mak
endef

include $(CONCERTO_ROOT)/combo_filters.mak

# Multi-core Build (Single Core is degenerative)
# This actually invokes the above macro
$(foreach TARGET_COMBO,$(TARGET_COMBOS),$(eval $(call CONCERTO_BUILD)))

ifndef NO_TARGETS
.PHONY: all dir depend build install uninstall clean clean_target outputs modules targets scrub vars test docs clean_docs pdf

depend::

all:: build

build:: dir depend

install:: build

uninstall::

outputs:: $(foreach mod,$(MODULES),$(mod)_output)

modules::
	$(PRINT) Modules are invokable makefile rules.
	$(foreach mod,$(MODULES),$(info MODULES+=$(mod)))

targets::
	$(PRINT) Concerto targets are the names of the binaries generated. 
	$(PRINT) Targets are invokable makefile rules. However, each combo version will be built.
	$(foreach target,$(CONCERTO_TARGETS),$(info CONCERTO_TARGETS+=$(target)))

scrub::
	$(if $(wildcard $(BUILD_OUTPUT)),$(info Deleting $(BUILD_OUTPUT)),$(info BUILD_OUTPUT does not exist!))
	$(if $(wildcard $(BUILD_OUTPUT)),-$(Q)$(CLEANDIR) $(call PATH_CONV,$(BUILD_OUTPUT)))

vars:: $(foreach mod,$(MODULES),$(mod)_vars)
	$(PRINT) HOST_ROOT=$(HOST_ROOT)
	$(PRINT) HOST_CPU=$(HOST_CPU)
	$(PRINT) HOST_FAMILY=$(HOST_FAMILY)
	$(PRINT) HOST_OS=$(HOST_OS)
	$(PRINT) MAKEFILE_LIST=$(MAKEFILE_LIST)
	$(PRINT) TARGET_MAKEFILES=$(TARGET_MAKEFILES)

test:: $(foreach mod,$(TESTABLE_MODULES),$(mod)_test)

todo:
	$(Q)fgrep -Rni TODO $(HOST_ROOT) --exclude-dir=.git \
									 --exclude-dir=.svn \
									 --exclude-dir=docs \
									 --exclude-dir=$(BUILD_FOLDER) \
									 --exclude-dir=$(BUILD_OUTPUT)

bugs:
	$(Q)fgrep -Rni "\bug" $(HOST_ROOT) --exclude-dir=.git \
									 --exclude-dir=.svn \
									 --exclude-dir=docs \
									 --exclude-dir=$(BUILD_FOLDER) \
									 --exclude-dir=$(BUILD_OUTPUT)

help: 
	$(PRINT)
	$(PRINT) Available top level rules:
	$(PRINT) " # Build all outputs."
	$(PRINT) " $$ $(MAKE) build"
	$(PRINT) " # Alias of 'build'"
	$(PRINT) " $$ $(MAKE) all"
	$(PRINT) " # Installs built outputs into system folders. May need 'root' privledge"
	$(PRINT) " $$ $(MAKE) install"
	$(PRINT) " # Uninstalls built outputs from system folders. May need 'root' privledge"
	$(PRINT) " $$ $(MAKE) install"
	$(PRINT) " # Removes build outputs only"
	$(PRINT) " $$ $(MAKE) clean"
	$(PRINT) " # Removes $(BUILD_OUTPUT)/ folder completely." 
	$(PRINT) " $$ $(MAKE) scrub"
	$(PRINT) " # Builds all Doxygen Documentation targets"
	$(PRINT) " $$ $(MAKE) docs"
	$(PRINT) " # Removes all Doxygen Documentation only." 
	$(PRINT) " $$ $(MAKE) clean_docs"
	$(PRINT) " # Shows build module variables" 
	$(PRINT) " $$ $(MAKE) vars"
	$(PRINT) " # Shows buildable modules" 
	$(PRINT) " $$ $(MAKE) modules"
	$(PRINT) " # Shows build targets variables" 
	$(PRINT) " $$ $(MAKE) targets"
	$(PRINT) " # Shows actual build output files" 
	$(PRINT) " $$ $(MAKE) outputs"
	$(PRINT) " # Shows any source lines which has a todo tag" 
	$(PRINT) " $$ $(MAKE) todo"
	$(PRINT) " # Shows any source lines which has a bug tag" 
	$(PRINT) " $$ $(MAKE) bugs"
	$(PRINT)
	$(PRINT) "Concerto Environment Variables (can be passed on command line too)"
	$(PRINT) "BUILD_DEBUG=1 - sets SHOW_COMMANDS and SHOW_MAKEDEBUG" 
	$(PRINT) "SHOW_COMMANDS=1 - shows all commands given the shell"
	$(PRINT) "SHOW_MAKEDEBUG=1 - shows extra makefile debugging"
	$(PRINT) "NO_OPTIMIZE=1 - Controls setting of TARGET_BUILD default. When set to '1', TARGET_BUILD=debug, else the default."
	$(PRINT) "FINAL_BUILD=1 - Puts build into production mode. All debug should be removed, optimizations on and symbols stripped"
	$(PRINT)
	$(PRINT) "Overridible Concerto Variables:"
	$(PRINT) "HOST_ROOT - the path to the root of build system to scan for $(SUBMAKEFILE). Defaults to the 'abspath' current directory."
	$(PRINT) "DIRECTORIES - defines which folders under HOST_ROOT which will be scanned for $(SUBMAKEFILE)"
	$(PRINT) "BUILD_FOLDER - the location of the concerto make folder under CONCERTO_ROOT. Defaults to 'concerto'."
	$(PRINT) "CONCERTO_ROOT - the path to the root of the concerto enabled build system. Default is 'HOST_ROOT'/'BUILD_FOLDER'"
	$(PRINT) "BUILD_TARGET - the location and name of the target specializing makefile. Defaults to 'CONCERTO_ROOT'/target.mak"
	$(PRINT) "BUILD_OUTPUT - the location to place the outputs of the build system. Defaults to 'out'"
	$(PRINT) "BUILD_PLATFORM - the location and name of the platform specializing makefile. Defaults to 'CONCERTO_ROOT'/platform.mak"
	$(PRINT) "TARGET_BUILD - Either 'release' (default) or 'debug'."
	$(PRINT) 

-include $(CONCERTO_ROOT)/project.mak

endif
