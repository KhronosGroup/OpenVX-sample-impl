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

ifneq ($(filter $(TARGET_OS),LINUX CYGWIN DARWIN),)

PKG_EXT := .tar.bz2

$(_MODULE)_PKG_NAME    := $(subst _,-,$($(_MODULE)_TARGET))
$(_MODULE)_PKG_FLDR    := $($(_MODULE)_TDIR)/$($(_MODULE)_PKG_NAME)
$(_MODULE)_PKG         := $($(_MODULE)_PKG_NAME)_$(TARGET_CPU)_$(VERSION)$(PKG_EXT)
$(_MODULE)_BIN         := $($(_MODULE)_TDIR)/$($(_MODULE)_PKG)

# Remember that the INSTALL variable tend to be based in /
$(_MODULE)_PKG_LIB := $($(_MODULE)_PKG_FLDR)$($(_MODULE)_INSTALL_LIB)
$(_MODULE)_PKG_INC := $($(_MODULE)_PKG_FLDR)$($(_MODULE)_INSTALL_INC)/$($(_MODULE)_INC_SUBPATH)
$(_MODULE)_PKG_BIN := $($(_MODULE)_PKG_FLDR)$($(_MODULE)_INSTALL_BIN)

ifeq ($(SHOW_MAKEDEBUG),1)
$(info $(_MODULE)_PKG_LIB=$($(_MODULE)_PKG_LIB))
$(info $(_MODULE)_PKG_INC=$($(_MODULE)_PKG_INC))
$(info $(_MODULE)_PKG_BIN=$($(_MODULE)_PKG_BIN))
$(info $(_MODULE)_PKG_CFG=$($(_MODULE)_PKG_CFG))
endif

$(_MODULE)_PKG_DEPS:= $(foreach lib,$($(_MODULE)_SHARED_LIBS),$($(_MODULE)_PKG_LIB)/lib$(lib).so) \
                      $(foreach lib,$($(_MODULE)_STATIC_LIBS),$($(_MODULE)_PKG_LIB)/lib$(lib).a) \
                      $(foreach bin,$($(_MODULE)_BINS),$($(_MODULE)_PKG_BIN)/$(bin)) \
                      $(foreach inc,$($(_MODULE)_INCS),$($(_MODULE)_PKG_INC)/$(notdir $(inc)))

$(_MODULE)_OBJS := $($(_MODULE)_PKG_DEPS)

define $(_MODULE)_PACKAGE

$(foreach lib,$($(_MODULE)_STATIC_LIBS),
$($(_MODULE)_PKG_LIB)/lib$(lib).a: $($(_MODULE)_TDIR)/lib$(lib).a $($(_MODULE)_PKG_LIB)/.gitignore
	$(Q)cp $$< $$@
)

$(foreach lib,$($(_MODULE)_SHARED_LIBS),
$($(_MODULE)_PKG_LIB)/lib$(lib).so: $($(_MODULE)_TDIR)/lib$(lib).so $($(_MODULE)_PKG_LIB)/.gitignore
	$(Q)cp $$< $$@
)

$(foreach bin,$($(_MODULE)_BINS),
$($(_MODULE)_PKG_BIN)/$(bin): $($(_MODULE)_TDIR)/$(bin) $($(_MODULE)_PKG_BIN)/.gitignore
	$(Q)cp $$< $$@
)

$(foreach inc,$($(_MODULE)_INCS),
$($(_MODULE)_PKG_INC)/$(notdir $(inc)): $(inc) $($(_MODULE)_PKG_INC)/.gitignore
	$(Q)cp $$< $$@
)

build:: $($(_MODULE)_BIN)

$($(_MODULE)_BIN): $($(_MODULE)_OBJS)
	$(Q)cd $($(_MODULE)_PKG_FLDR) && tar cjf $$(notdir $$@) $$(subst $($(_MODULE)_PKG_FLDR),.,$$^)
	@echo Packaging $$(notdir $$@)
endef

else
# This prevents non-tar system from worrying about packages
$(_MODULE)_BIN :=
endif

