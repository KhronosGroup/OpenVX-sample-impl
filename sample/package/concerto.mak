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


# Make a DEB package out of OpenVX Sample Implementation
ifeq ($(HOST_OS),LINUX)
include $(PRELUDE)
TARGET      := openvx-$(SCM_VERSION)
TARGETTYPE  := deb
STATIC_LIBS := openvx-debug-lib openvx-extras-lib vx_xyz_lib
SHARED_LIBS := openvx vxu openvx-debug openvx-extras xyz
BINS        := vx_test
INCS        := $(wildcard include/VX/*.h)
INC_SUBPATH := VX
include $(FINALE)
endif
