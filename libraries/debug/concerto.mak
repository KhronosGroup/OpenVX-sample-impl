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


_MODULE     := openvx-debug-lib
include $(PRELUDE)
TARGET      := openvx-debug-lib
TARGETTYPE  := library
CSOURCES    := vx_debug_lib.c
include $(FINALE)

_MODULE     := openvx-debug
include $(PRELUDE)
TARGET      := openvx-debug
TARGETTYPE  := dsmo
IDIRS       += $(HOST_ROOT)/kernels/debug
DEFFILE     := openvx-debug.def
CSOURCES    := $(filter-out vx_debug_lib.c,$(call all-c-files))
STATIC_LIBS := openvx-helper openvx-debug_k-lib
SHARED_LIBS := openvx
include $(FINALE)

