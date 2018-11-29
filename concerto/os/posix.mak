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

$(_MODULE)_INSTALL_DSO   := $(INSTALL) $($(_MODULE)_BIN).$($(_MODULE)_VERSION) $($(_MODULE)_INSTALL_LIB)
$(_MODULE)_UNINSTALL_DSO := $(CLEAN) $($(_MODULE)_INSTALL_LIB)/$($(_MODULE)_OUT)
$(_MODULE)_UNLN_DSO      := $(CLEAN) $($(_MODULE)_BIN).$($(_MODULE)_VERSION)
$(_MODULE)_UNLN_INST_DSO := $(CLEAN) $($(_MODULE)_INSTALL_LIB)/$($(_MODULE)_OUT).$($(_MODULE)_VERSION)

define $(_MODULE)_UNINSTALL
uninstall::
	@echo Uninstalling $($(_MODULE)_BIN) from $($(_MODULE)_INSTALL_LIB)
	-$(Q)$(call $(_MODULE)_UNLN_INST_DSO)
	-$(Q)$(call $(_MODULE)_UNINSTALL_DSO)
endef

define $(_MODULE)_INSTALL
install:: $($(_MODULE)_BIN)
	@echo Installing $($(_MODULE)_BIN) to $($(_MODULE)_INSTALL_LIB)
	-$(Q)$(call $(_MODULE)_INSTALL_DSO)
	-$(Q)$(call $(_MODULE)_LN_INST_DSO)
endef

define $(_MODULE)_CLEAN_LNK
clean::
	@echo Removing Link for Shared Object $($(_MODULE)_BIN).$($(_MODULE)_VERSION)
	-$(Q)$(call $(_MODULE)_UNLN_DSO)
endef

else ifeq ($($(_MODULE)_TYPE),exe)

$(_MODULE)_ATTRIB_EXE    := $(SET_EXEC) $($(_MODULE)_BIN)
$(_MODULE)_INSTALL_EXE   := $(INSTALL) $($(_MODULE)_BIN) $($(_MODULE)_INSTALL_BIN)
$(_MODULE)_UNINSTALL_EXE := $(CLEAN) $($(_MODULE)_INSTALL_BIN)/$($(_MODULE)_OUT)

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
