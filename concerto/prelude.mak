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

# Take the Makefile list and remove prelude and finale includes
# @note MAKEFILE_LIST is an automatic variable!

# Get all the Concerto files (can't use TARGET_MAKEFILES since we don't know the ordering)
CONCERTO_MAKEFILES := $(filter %/$(SUBMAKEFILE),$(MAKEFILE_LIST))
THIS_MAKEFILE := $(lastword $(CONCERTO_MAKEFILES))
_MODPATH := $(subst /$(SUBMAKEFILE),,$(THIS_MAKEFILE))
_MODDIR := $(subst /,.,$(_MODPATH))

$(if $(SHOW_MAKEDEBUG),$(info HOST_ROOT=$(HOST_ROOT)))
$(if $(SHOW_MAKEDEBUG),$(info MAKEFILE_LIST (so far) = $(MAKEFILE_LIST)))
$(if $(SHOW_MAKEDEBUG),$(info CONCERTO_MAKEFILES=$(CONCERTO_MAKEFILES)))
$(if $(SHOW_MAKEDEBUG),$(info THIS_MAKEFILE=$(THIS_MAKEFILE)))
$(if $(SHOW_MAKEDEBUG),$(info _MODPATH=$(_MODPATH)))
$(if $(SHOW_MAKEDEBUG),$(info _MODDIR=$(_MODDIR)))

# Error if empty
$(if $(_MODPATH),,$(error $(THIS_MAKEFILE) failed to get module path))

# if the makefile didn't define the module name, use the directory name
ifeq ($(BUILD_MULTI_PROJECT),1)
ifneq ($(_MODULE),)
_MODULE:=$(_MODDIR)+$(_MODULE)
else
_MODULE:=$(_MODDIR)
endif
else ifeq ($(_MODULE),) # not multiproject and _MODULE was not set
_MODULE:=$(_MODDIR)
endif
$(if $(SHOW_MAKEDEBUG),$(info _MODULE=$(_MODULE)))

# if there's no module name, fail
$(if $(_MODULE),,$(error Failed to create module name!))

# Save the short name of the module
_MODULE_NAME := $(_MODULE)

ifneq ($(TARGET_COMBOS),)
# Append differentiation if in multi-core mode
_MODULE := $(_MODULE).$(TARGET_PLATFORM).$(TARGET_OS).$(TARGET_CPU).$(TARGET_BUILD)
endif

# Print some info to show that we're processing the makefiles
$(if $(SHOW_MAKEDEBUG),$(info Adding Module $(_MODULE) to MODULES))

# IF there is a conflicting _MODULE, error
$(if $(filter $(_MODULE),$(MODULES)),$(error MODULE $(_MODULE) already defined in $(MODULES)!))

# Add the current module to the modules list
MODULES += $(_MODULE)

# Define the Path to the Source Files (always use the directory) and Header Files
$(_MODULE)_SDIR := $(HOST_ROOT)/$(_MODPATH)
$(_MODULE)_IDIRS:= $($(_MODULE)_SDIR)

# Route the output for each module into it's own folder
$(_MODULE)_ODIR := $(TARGET_OUT)/module/$(_MODULE_NAME)
$(_MODULE)_TDIR := $(TARGET_OUT)

# Set the initial linking directories to the target directory
$(_MODULE)_LDIRS := $($(_MODULE)_TDIR)

# Set the install directory if it's not set already
ifndef INSTALL_LIB
$(_MODULE)_INSTALL_LIB := $($(_MODULE)_TDIR)
else
$(_MODULE)_INSTALL_LIB := $(INSTALL_LIB)
endif
ifndef INSTALL_BIN
$(_MODULE)_INSTALL_BIN := $($(_MODULE)_TDIR)
else
$(_MODULE)_INSTALL_BIN := $(INSTALL_BIN)
endif
ifndef INSTALL_INC
$(_MODULE)_INSTALL_INC := $($(_MODULE)_TDIR)/include
else
$(_MODULE)_INSTALL_INC := $(INSTALL_INC)
endif

# Define a ".gitignore" file which will help in making sure the module's output
# folder always exists.
%.gitignore:
	-$(Q)$(MKDIR) $(call PATH_CONV,$(dir $@)) $(QUIET)
	-$(Q)$(TOUCH) $(call PATH_CONV,$@)
	
dir:: $($(_MODULE)_ODIR)/.gitignore

# Clean out the concerto.mak variables
_MODULE_VARS := ENTRY DEFS CFLAGS LDFLAGS STATIC_LIBS SHARED_LIBS SYS_STATIC_LIBS
_MODULE_VARS += SYS_SHARED_LIBS IDIRS LDIRS CSOURCES CPPSOURCES ASSEMBLY JSOURCES
_MODULE_VARS += KCSOURCES JAVA_LIBS TARGET TARGETTYPE BINS INCS INC_SUBPATH HEADERS
_MODULE_VARS += MISRA_RULES LINKER_FILES DEFFILE PDFNAME TESTPRGM TESTOPTS VERSION
_MODULE_VARS += SKIPBUILD XDC_GOALS XDC_SUFFIXES XDC_PROFILES XDC_ARGS NOT_PKGS XDC_PACKAGES
_MODULE_VARS += USE_GLUT USE_OPENCL USE_OPENMP TESTPATH 
_MODULE_VARS += BSTFILES STYFILES IMGFILES TEXFILES DOTFILES MSCFILES BIBFILES

_MODULE_VARS := $(sort $(_MODULE_VARS))

ifeq ($(SHOW_MAKEDEBUG),1)
$(info _MODULE_VARS=$(_MODULE_VARS))
endif

# Clear out all the variables
ifneq ($(filter $(MAKE_VERSION),3.82 4.0),)
$(foreach mvar,$(_MODULE_VARS),$(eval undefine $(mvar)))
else
$(foreach mvar,$(_MODULE_VARS),$(eval $(mvar):=$(EMPTY)))
endif

# Clearing all "MODULE_%" variables
$(foreach mod,$(filter MODULE_%,$(.VARIABLES)),$(eval $(mod):=$(EMPTY)))

# Define convenience variables
SDIR := $($(_MODULE)_SDIR)
TDIR := $($(_MODULE)_TDIR)
ODIR := $($(_MODULE)_ODIR)

# Pull in the definitions which will be redefined for this makefile
include $(CONCERTO_ROOT)/definitions.mak
