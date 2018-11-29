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


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := $(OPENVX_DEFS)
LOCAL_SRC_FILES := vx_test.c
LOCAL_C_INCLUDES := $(OPENVX_INC) \
	$(OPENVX_TOP)/examples \
	$(OPENVX_TOP)/$(OPENVX_SRC)/include \
	$(OPENVX_TOP)/$(OPENVX_SRC)/extensions/include
LOCAL_STATIC_LIBRARIES := libvx_xyz_lib libopenvx-debug-lib libopenvx-extras-lib libopenvx-helper
LOCAL_SHARED_LIBRARIES := libdl libutils libcutils libbinder libhardware libion libgui libui libopenvx libvxu
LOCAL_MODULE := vx_test
include $(BUILD_EXECUTABLE)
