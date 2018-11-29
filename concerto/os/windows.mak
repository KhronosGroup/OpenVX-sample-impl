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

ifeq ($($(_MODULE)_TYPE),dsmo)

$(_MODULE)_INSTALL_DSO   := $(COPY) $(call PATH_CONV,$($(_MODULE)_TDIR)\\$($(_MODULE)_BIN)) $(call PATH_CONV,$($(_MODULE)_INSTALL_LIB))
$(_MODULE)_UNINSTALL_DSO := $(CLEAN) $(call PATH_CONV,$($(_MODULE)_INSTALL_LIB)\\$($(_MODULE)_OUT))
$(_MODULE)_LN_DS0        := $(SET_RW) $(call PATH_CONV,$($(_MODULE)_INSTALL_LIB)\\$($(_MODULE)_OUT))
$(_MODULE)_UNLN_DSO      := $(SET_RW) $(call PATH_CONV,$($(_MODULE)_INSTALL_LIB)\\$($(_MODULE)_OUT))

define $(_MODULE)_UNINSTALL
uninstall::
	@echo Uninstalling $($(_MODULE)_BIN) from $($(_MODULE)_INSTALL_LIB)
	-$(Q)$(call $(_MODULE)_UNLN_DSO)
	-$(Q)$(call $(_MODULE)_UNINSTALL_DSO)
endef

define $(_MODULE)_INSTALL
install:: $($(_MODULE)_BIN)
	@echo Installing $($(_MODULE)_BIN) to $($(_MODULE)_INSTALL_LIB)
	-$(Q)$(call $(_MODULE)_INSTALL_DSO)
	-$(Q)$(call $(_MODULE)_LN_DSO)
endef

# Windows does not support soft-linking versions
define $(_MODULE)_CLEAN_LNK
clean::
endef

else ifeq ($($(_MODULE)_TYPE),exe)

$(_MODULE)_ATTRIB_EXE := $(ATTRIB) $(call PATH_CONV,$($(_MODULE)_BIN))
$(_MODULE)_INSTALL_EXE := $(COPY) $(call PATH_CONV,$($(_MODULE)_TDIR)\\$($(_MODULE)_BIN)) $(call PATH_CONV,$($(_MODULE)_INSTALL_BIN))
$(_MODULE)_UNINSTALL_EXE := $(CLEAN) $(call PATH_CONV,$($(_MODULE)_INSTALL_BIN)\\$($(_MODULE)_BIN))

define $(_MODULE)_UNINSTALL
uninstall::
	@echo Uninstalling $($(_MODULE)_BIN) from $($(_MODULE)_INSTALL_BIN)
	-$(Q)$(call $(_MODULE)_UNINSTALL_EXE)
endef

define $(_MODULE)_INSTALL
install:: $($(_MODULE)_BIN)
	@echo Installing $($(_MODULE)_BIN) to $($(_MODULE)_INSTALL_BIN)
	-$(Q)$(call $(_MODULE)_INSTALL_EXE)
	-$(Q)$(call $(_MODULE)_ATTRIB_EXE)
endef

endif

