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

# Set the default values if not defined or empty string
ifeq ($(VERSION),)
VERSION := $(strip $(shell $(CAT) $(CONCERTO_ROOT)/../VERSION))
endif

-include $(CONCERTO_ROOT)/components/glut.mak
-include $(CONCERTO_ROOT)/components/opencl.mak
-include $(CONCERTO_ROOT)/components/openmp.mak

# Users may use the new syntax of all variables starting with "MODULE_" but may not quite
# yet so don't erase the value if it's set.
#$(foreach mvar,$(_MODULE_VARS),$(if $(MODULE_$(mvar)),,$(eval MODULE_$(mvar):=$(value $(mvar)))))

ifeq ($(SHOW_MAKEDEBUG),1)
$(info #### $(_MODULE) ########################################################)
$(foreach mvar,$(sort $(_MODULE_VARS)),$(if $(value $(mvar)),$(info $(mvar)=$(value $(mvar)))))
$(foreach mvar,$(sort $(filter MODULE_%,$(.VARIABLES))),$(if $(value $(mvar)),$(info $(mvar)=$(value $(mvar)))))
endif

$(_MODULE)_TARGET := $(TARGET)
CONCERTO_TARGETS += $(if $(filter $(CONCERTO_TARGETS),$(TARGET)),,$($(_MODULE)_TARGET))

# Add the paths from the makefile
$(_MODULE)_IDIRS += $(SYSIDIRS) $(IDIRS)
$(_MODULE)_LDIRS += $(LDIRS) $($(TARGET_COMBO_NAME)_LDIRS) $(SYSLDIRS)

# Add any additional libraries which are in this build system
$(_MODULE)_STATIC_LIBS += $(STATIC_LIBS)
$(_MODULE)_SHARED_LIBS += $(SHARED_LIBS)
$(_MODULE)_SYS_STATIC_LIBS += $(SYS_STATIC_LIBS)
$(_MODULE)_SYS_SHARED_LIBS += $(SYS_SHARED_LIBS)
$(_MODULE)_JAVA_LIBS += $(JAVA_LIBS)
$(_MODULE)_BINS += $(BINS)
$(_MODULE)_INCS += $(INCS)
ifdef INC_SUBPATH
$(_MODULE)_INC_SUBPATH := $(INC_SUBPATH)
else
$(_MODULE)_INC_SUBPATH := $(TARGET)
endif
$(_MODULE)_HEADERS := $(HEADERS)

# Copy over the rest of the variables
$(_MODULE)_TYPE := $(strip $(TARGETTYPE))
$(_MODULE)_TDEFS := $(TARGET_DEFS)
$(_MODULE)_DEFS := $(SYSDEFS) $(DEFS) $($(TARGET_COMBO_NAME)_DEFS)
$(_MODULE)_TESTPRGM := $(TESTPRGM)
$(_MODULE)_TESTOPTS := $(TESTOPTS)
$(_MODULE)_TESTPATH := $(TESTPATH)
$(_MODULE)_VERSION := $(VERSION)

# Set the Install Path
$(_MODULE)_INSTALL_PATH = $(INSTALL_PATH)

# For debugging the build system
$(_MODULE)_SRCS := $(CSOURCES) $(CPPSOURCES) $(ASSEMBLY) $(JSOURCES)

ifneq ($(SKIPBUILD),1)

NEEDS_COMPILER:=
NEEDS_INSTALL := 
ifeq ($($(_MODULE)_TYPE),library)
	NEEDS_COMPILER=1
	# no install for libs
else ifeq ($($(_MODULE)_TYPE),dsmo)
	NEEDS_COMPILER=1
	NEEDS_INSTALL=1
else ifeq ($($(_MODULE)_TYPE),exe)
	NEEDS_COMPILER=1
	NEEDS_INSTALL=1
else ifeq ($($(_MODULE)_TYPE),jar)
	include $(CONCERTO_ROOT)/compilers/java.mak
else ifeq ($($(_MODULE)_TYPE),deb)
    include $(CONCERTO_ROOT)/tools/dpkg.mak
else ifeq ($($(_MODULE)_TYPE),tar)
    include $(CONCERTO_ROOT)/tools/tar.mak 
else ifeq ($($(_MODULE)_TYPE),doxygen)
    include $(CONCERTO_ROOT)/tools/doxygen.mak
else ifeq ($($(_MODULE)_TYPE),latex)
	include $(CONCERTO_ROOT)/tools/latex.mak
# \todo add new build types here!    
endif

ifeq ($(NEEDS_COMPILER),1)
# which compiler does this need?
ifeq ($(HOST_COMPILER),GCC)
	include $(CONCERTO_ROOT)/compilers/gcc.mak
else ifeq ($(HOST_COMPILER),CLANG)
	include $(CONCERTO_ROOT)/compilers/clang.mak
else ifeq ($(HOST_COMPILER),CL)
	include $(CONCERTO_ROOT)/compilers/cl.mak
else ifeq ($(HOST_COMPILER),RVCT)
	include $(CONCERTO_ROOT)/compilers/rvct.mak
else
$(error Undefined compiler $(HOST_COMPILER))	
endif
endif

ifeq ($(NEEDS_INSTALL),1)
ifeq ($(TARGET_OS),Windows_NT)
    include $(CONCERTO_ROOT)/os/windows.mak
else 
    include $(CONCERTO_ROOT)/os/posix.mak
endif    
endif

else # SKIPBUILD=1

$(info Build Skipped for $(_MODULE):$($(_MODULE)_TARGET))

endif  # ifneq SKIPBUILD

# Reset Skipbuild
SKIPBUILD:=

###################################################
# RULES
###################################################

.PHONY: $(_MODULE)_output
define $(_MODULE)_output_list
$(_MODULE)_output:: 
	@echo $(1)
endef

$(foreach bin,$($(_MODULE)_BIN),$(eval $(call $(_MODULE)_output_list,$(bin))))

.PHONY: $(TARGET)
$(TARGET):: $(_MODULE)

.PHONY: $(_MODULE)
$(_MODULE):: $($(_MODULE)_BIN)

ifneq ($($(_MODULE)_TESTPRGM),)
TESTABLE_MODULES += $(_MODULE)
.PHONY: $(_MODULE)_test

define $(_MODULE)_TESTCASE
$(_MODULE)_test: $($(_MODULE)_TDIR)/$($(_MODULE)_TESTPRGM)
	$(PRINT) Running $($(_MODULE)_TESTPRGM)
ifeq ($(HOST_OS),Windows_NT)
	$(Q)cd $($(_MODULE)_TESTPATH) && PATH=$(PATH)\;$($(_MODULE)_TDIR) $($(_MODULE)_TDIR)/$($(_MODULE)_TESTPRGM) $($(_MODULE)_TESTOPTS)
else
	$(Q)cd $($(_MODULE)_TESTPATH) && LD_LIBRARY_PATH=$($(_MODULE)_TDIR) $($(_MODULE)_TDIR)/$($(_MODULE)_TESTPRGM) $($(_MODULE)_TESTOPTS)
endif

endef

$(eval $(call $(_MODULE)_TESTCASE))
endif

ifeq ($($(_MODULE)_TYPE),library)

define $(_MODULE)_BUILD_LIB
$($(_MODULE)_BIN): $($(_MODULE)_OBJS) $($(_MODULE)_STATIC_LIBS)
	$(PRINT) Linking $$@
	-$(Q)$(call $(_MODULE)_LINK_LIB) $(LOGGING)
endef

$(eval $(call $(_MODULE)_BUILD_LIB))
$(eval $(call $(_MODULE)_INSTALL))
$(eval $(call $(_MODULE)_BUILD))
$(eval $(call $(_MODULE)_UNINSTALL))

else ifeq ($($(_MODULE)_TYPE),exe)

define $(_MODULE)_BUILD_EXE
$($(_MODULE)_BIN): $($(_MODULE)_OBJS) $($(_MODULE)_STATIC_LIBS) $($(_MODULE)_SHARED_LIBS) $($(_MODULE)_DEPS)
	$(PRINT) Linking $$@
	-$(Q)$(call $(_MODULE)_LINK_EXE) $(LOGGING)
endef

$(eval $(call $(_MODULE)_BUILD_EXE))
$(eval $(call $(_MODULE)_INSTALL))
$(eval $(call $(_MODULE)_BUILD))
$(eval $(call $(_MODULE)_UNINSTALL))

else ifeq ($($(_MODULE)_TYPE),dsmo)

define $(_MODULE)_BUILD_DSO
$($(_MODULE)_BIN): $($(_MODULE)_OBJS) $($(_MODULE)_STATIC_LIBS) $($(_MODULE)_SHARED_LIBS) $($(_MODULE)_DEPS)
	$(PRINT) Linking $$@
	$(Q)$(call $(_MODULE)_LINK_DSO) $(LOGGING)
	-$(Q)$(call $(_MODULE)_LN_DSO)
endef

$(eval $(call $(_MODULE)_BUILD_DSO))
$(eval $(call $(_MODULE)_INSTALL))
$(eval $(call $(_MODULE)_BUILD))
$(eval $(call $(_MODULE)_UNINSTALL))

else ifeq ($($(_MODULE)_TYPE),objects)

$($(_MODULE)_BIN): $($(_MODULE)_OBJS)

else ifeq ($($(_MODULE)_TYPE),prebuilt)

$(_MODULE)_BIN := $($(_MODULE)_TDIR)/$(TARGET)$(suffix $(PREBUILT))
build:: $($(_MODULE)_BIN)

#In Windows, copy does not update the timestamp, so we have to do an extra step below to update the timestamp
define $(_MODULE)_PREBUILT
$($(_MODULE)_BIN): $($(_MODULE)_SDIR)/$(1) $($(_MODULE)_TDIR)/.gitignore
	@echo Copying Prebuilt binary $($(_MODULE)_SDIR)/$(1) to $($(_MODULE)_BIN)
	-$(Q)$(COPY) $(call PATH_CONV,$($(_MODULE)_SDIR)/$(1) $($(_MODULE)_BIN))
ifeq ($(HOST_OS),Windows_NT)
	-$(Q)cd $($(_MODULE)_TDIR) && $(COPY) $(call PATH_CONV,$($(_MODULE)_BIN))+,,
endif

$(_MODULE)_CLEAN_LNK =

endef

$(eval $(call $(_MODULE)_PREBUILT,$(PREBUILT)))

else ifeq ($($(_MODULE)_TYPE),jar)

$(eval $(call $(_MODULE)_DEPEND_JAR))

else ifneq ($(filter $($(_MODULE)_TYPE),deb tar),)

$(eval $(call $(_MODULE)_PACKAGE))

else ifeq ($($(_MODULE)_TYPE),doxygen)

$(eval $(call $(_MODULE)_DOCUMENTS))

else ifeq ($($(_MODULE)_TYPE),latex)

# $($(_MODULE)_COMPILE_TOOLS) will pickup the rules

endif

define $(_MODULE)_PLATFORM_DEPS
.PHONY: $($(_MODULE)_PLATFORM_LIBS)
endef

$(eval $(call $(_MODULE)_PLATFORM_DEPS))

define $(_MODULE)_CLEAN
.PHONY: clean_target clean $(_MODULE)_clean $(_MODULE)_clean_target
clean_target:: $(_MODULE)_clean_target
clean:: $(_MODULE)_clean

$(_MODULE)_clean_target::
	$(PRINT) Cleaning $(_MODULE) target $($(_MODULE)_BIN)
	-$(Q)$(CLEAN) $(call PATH_CONV,$($(_MODULE)_BIN))
ifneq ($($(_MODULE)_MAP),)
	$(PRINT) Cleaning $(_MODULE) target mapfile $($(_MODULE)_MAP)
	-$(Q)$(CLEAN) $(call PATH_CONV,$($(_MODULE)_MAP))
endif

$(_MODULE)_clean:: $(_MODULE)_clean_target
	$(PRINT) Cleaning $($(_MODULE)_ODIR)
	-$(Q)$(CLEANDIR) $(call PATH_CONV,$($(_MODULE)_ODIR))

endef

# Include compiler generated dependency rules
# Second rule is to support module source sub-directories
define $(_MODULE)_DEPEND
-include $(1).dep
$(1)$(2): $(dir $(1)).gitignore
endef

$(foreach obj,$($(_MODULE)_OBJS),$(eval $(call $(_MODULE)_DEPEND,$(basename $(obj)),$(suffix $(obj)))))
$(eval $(call $(_MODULE)_CLEAN))
$(eval $(call $(_MODULE)_CLEAN_LNK))
$(eval $(call $(_MODULE)_COMPILE_TOOLS))
$(eval $(call $(_MODULE)_ANALYZER))

define $(_MODULE)_VARDEF
$(_MODULE)_vars::
	$(PRINT) =============================================
	$(PRINT) _MODULE=$(_MODULE)
	$(PRINT) $(_MODULE)_TARGET=$($(_MODULE)_TARGET)
	$(PRINT) $(_MODULE)_BIN =$($(_MODULE)_BIN)
	$(PRINT) $(_MODULE)_TYPE=$($(_MODULE)_TYPE)
	$(PRINT) $(_MODULE)_OBJS=$($(_MODULE)_OBJS)
	$(PRINT) $(_MODULE)_SDIR=$($(_MODULE)_SDIR)
	$(PRINT) $(_MODULE)_ODIR=$($(_MODULE)_ODIR)
	$(PRINT) $(_MODULE)_TDIR=$($(_MODULE)_TDIR)
	$(PRINT) $(_MODULE)_SRCS=$($(_MODULE)_SRCS)
	$(PRINT) $(_MODULE)_IDIRS=$($(_MODULE)_IDIRS)
	$(PRINT) $(_MODULE)_LDIRS=$($(_MODULE)_LDIRS)
	$(PRINT) $(_MODULE)_STATIC_LIBS=$($(_MODULE)_STATIC_LIBS)
	$(PRINT) $(_MODULE)_SHARED_LIBS=$($(_MODULE)_SHARED_LIBS)
	$(PRINT) $(_MODULE)_SYS_STATIC_LIBS=$($(_MODULE)_SYS_STATIC_LIBS)
	$(PRINT) $(_MODULE)_SYS_SHARED_LIBS=$($(_MODULE)_SYS_SHARED_LIBS)
	$(PRINT) $(_MODULE)_CLASSES=$($(_MODULES)_CLASSES)
	$(PRINT) $(_MODULE)_TESTPRGM=$($(_MODULE)_TESTPRGM)
	$(PRINT) $(_MODULE)_TESTOPTS=$($(_MODULE)_TESTOPTS)
	$(PRINT) $(_MODULE)_TESTPATH=$($(_MODULE)_TESTPATH)
	$(PRINT) $(_MODULE)_DEFS=$($(_MODULE)_DEFS)
	$(PRINT) =============================================
endef

$(eval $(call $(_MODULE)_VARDEF))

ifeq ($(SHOW_MAKEDEBUG),1)
$(foreach mvar,$(sort $(filter $(_MODULE)_%,$(.VARIABLES))),$(if $(value $(mvar)),$(info $(mvar)=$(value $(mvar)))))
$(info #### $(_MODULE) ########################################################)
endif

# Now clear out the module variable for repeat definitions
_MODULE :=

