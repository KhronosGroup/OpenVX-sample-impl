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


ifeq ($(BOARD_USES_OPENVX),1)

OPENVX_TOP := $(call my-dir)
OPENVX_INC := $(OPENVX_TOP)/include
OPENVX_DEFS:= -D_LITTLE_ENDIAN_ \
			  -DEXPERIMENTAL_USE_DOT
OPENVX_SRC := sample
OPENVX_DIRS := $(OPENVX_SRC) examples conformance helper debug libraries kernels tools
$(foreach dir,$(OPENVX_DIRS),$(eval include $(OPENVX_TOP)/$(dir)/Android.mk))

endif
