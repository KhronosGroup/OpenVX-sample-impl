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

ifeq ($(TARGET_OS),LINUX)

CHECK_DPKG := $(shell which dpkg)
ifeq ($(SHOW_MAKEDEBUG),1)
$(info CHECK_DPKG=$(CHECK_DPKG))
endif
ifneq ($(CHECK_DPKG),)

PKG_EXT := .deb

VARS=$(shell "dpkg-architecture")
$(foreach var,$(VARS),$(if $(findstring DEB_BUILD_ARCH,$(var)),$(eval $(var))))

$(_MODULE)_CFG         ?= control
$(_MODULE)_PKG_NAME    := $(subst _,-,$($(_MODULE)_TARGET))
$(_MODULE)_PKG_FLDR    := $($(_MODULE)_TDIR)/$($(_MODULE)_PKG_NAME)
$(_MODULE)_PKG         := $($(_MODULE)_PKG_NAME)$(PKG_EXT)
$(_MODULE)_BIN         := $($(_MODULE)_TDIR)/$($(_MODULE)_PKG)

ifeq ($(SHOW_MAKEDEBUG),1)
$(info $(_MODULE)_PKG_FLDR=$($(_MODULE)_PKG_FLDR))
endif

# Remember that the INSTALL variable tend to be based in /
$(_MODULE)_PKG_LIB := $($(_MODULE)_PKG_FLDR)$($(_MODULE)_INSTALL_LIB)
$(_MODULE)_PKG_INC := $($(_MODULE)_PKG_FLDR)$($(_MODULE)_INSTALL_INC)/$($(_MODULE)_INC_SUBPATH)
$(_MODULE)_PKG_BIN := $($(_MODULE)_PKG_FLDR)$($(_MODULE)_INSTALL_BIN)
$(_MODULE)_PKG_CFG := $($(_MODULE)_PKG_FLDR)/DEBIAN

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

ifeq ($(SHOW_MAKEDEBUG),1)
$(info $(_MODULE)_PKG_DEPS=$($(_MODULE)_PKG_DEPS))
endif

$(_MODULE)_OBJS := $($(_MODULE)_PKG_CFG)/$($(_MODULE)_CFG) $($(_MODULE)_PKG_DEPS)

ifeq ($(SHOW_MAKEDEBUG),1)
$(info $(_MODULE)_OBJS=$($(_MODULE)_OBJS))
endif

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

$($(_MODULE)_PKG_CFG)/$($(_MODULE)_CFG) : $($(_MODULE)_SDIR)/$($(_MODULE)_CFG) $($(_MODULE)_PKG_CFG)/.gitignore
	$(Q)echo "Package: $($(_MODULE)_PKG_NAME)" > $$@
	$(Q)cat $($(_MODULE)_SDIR)/$($(_MODULE)_CFG) >> $$@
	$(Q)echo "Architecture: $(DEB_BUILD_ARCH)" >> $$@

build:: $($(_MODULE)_BIN)

$($(_MODULE)_BIN): $($(_MODULE)_OBJS)
	$(Q)dpkg --build $$(basename $$@)
endef

else
# This prevents non-dpkg system from worrying about packages
$(_MODULE)_BIN :=
endif
endif

