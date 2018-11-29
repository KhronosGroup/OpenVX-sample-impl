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
LOCAL_SRC_FILES := \
	vx_context.c \
	vx_convolution.c \
	vx_delay.c \
	vx_distribution.c \
	vx_error.c \
	vx_graph.c \
	vx_image.c \
	vx_kernel.c \
	vx_list.c \
	vx_log.c \
	vx_lut.c \
	vx_matrix.c \
	vx_memory.c \
	vx_node_api.c \
	vx_node.c \
	vx_osal.c \
	vx_parameter.c \
	vx_pyramid.c \
	vx_reference.c \
	vx_remap.c \
	vx_scalar.c \
	vx_target.c \
	vx_threshold.c
LOCAL_C_INCLUDES := $(OPENVX_INC) $(OPENVX_TOP)/$(OPENVX_SRC)/include $(OPENVX_TOP)/debug
LOCAL_WHOLE_STATIC_LIBRARIES := libopenvx-helper libopenvx-debug
LOCAL_SHARED_LIBRARIES := libdl libutils libcutils libbinder libhardware libion libgui libui
LOCAL_MODULE := libopenvx
include $(BUILD_SHARED_LIBRARY)

