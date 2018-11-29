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


include $(PRELUDE)
TARGET      := openvx
DEFFILE     := openvx.def
TARGETTYPE  := dsmo
IDIRS       += $(HOST_ROOT)/$(OPENVX_SRC)/include $(HOST_ROOT)/debug $(XML2_INC)
CSOURCES    :=  $(call all-c-files)
STATIC_LIBS := openvx-helper vx_debug utils-lib
SYS_SHARED_LIBS += $(XML2_LIBS)
include $(FINALE)
