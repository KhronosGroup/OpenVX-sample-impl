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


_MODULE     := vx_test
include $(PRELUDE)
TARGET      := vx_test
TARGETTYPE  := exe
CSOURCES    := $(TARGET).c
SHARED_LIBS := openvx vxu
STATIC_LIBS := vx_xyz_lib openvx-debug-lib openvx-extras-lib openvx-helper
IDIRS       += $(HOST_ROOT)/examples $(HOST_ROOT)/$(OPENVX_SRC)/include
TESTPRGM    := $(TARGET)
TESTOPTS    := 
TESTPATH    := raw
include $(FINALE)

_MODULE     := vx_bug13510
include $(PRELUDE)
TARGET      := vx_bug13510
TARGETTYPE  := exe
CSOURCES    := $(TARGET).c
SHARED_LIBS := openvx vxu
STATIC_LIBS := openvx-debug-lib openvx-extras-lib openvx-helper
IDIRS       += $(HOST_ROOT)/examples $(HOST_ROOT)/$(OPENVX_SRC)/include
TESTPRGM    := $(TARGET)
TESTOPTS    := 
TESTPATH    := raw
include $(FINALE)

_MODULE     := vx_bug13517
include $(PRELUDE)
TARGET      := vx_bug13517
TARGETTYPE  := exe
CSOURCES    := $(TARGET).c
SHARED_LIBS := openvx vxu
STATIC_LIBS := openvx-debug-lib openvx-extras-lib openvx-helper
IDIRS       += $(HOST_ROOT)/examples $(HOST_ROOT)/$(OPENVX_SRC)/include
TESTPRGM    := $(TARGET)
TESTOPTS    := 
TESTPATH    := raw
include $(FINALE)

_MODULE     := vx_bug13518
include $(PRELUDE)
TARGET      := vx_bug13518
TARGETTYPE  := exe
CSOURCES    := $(TARGET).c
SHARED_LIBS := openvx vxu
STATIC_LIBS := openvx-debug-lib openvx-extras-lib openvx-helper
IDIRS       += $(HOST_ROOT)/examples $(HOST_ROOT)/$(OPENVX_SRC)/include
TESTPRGM    := $(TARGET)
TESTOPTS    := 
TESTPATH    := raw
include $(FINALE)

ifeq ($(TARGET_OS),LINUX)
_MODULE     := vx_cam_test
include $(PRELUDE)
ifeq ($(filter OPENVX_USE_SDL,$(SYSDEFS)),)
SKIPBUILD   := 1
endif
TARGET      := vx_cam_test
TARGETTYPE  := exe
CSOURCES    := $(TARGET).c
SHARED_LIBS := openvx vxu $(SDL_LIBS)
STATIC_LIBS := openvx-debug-lib openvx-extras-lib openvx-helper
IDIRS       += $(HOST_ROOT)/examples $(HOST_ROOT)/$(OPENVX_SRC)/include
TESTPRGM    := $(TARGET)
TESTOPTS    := 
TESTPATH    := raw
include $(FINALE)
endif