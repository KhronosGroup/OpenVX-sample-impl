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
LOCAL_SRC_FILES :=  c_absdiff.c \
					c_accumulate.c \
					c_addsub.c \
					c_bitwise.c \
					c_channel.c \
					c_conv3x3.c \
					c_convertcolor.c \
					c_convertdepth.c \
					c_convolve.c \
					c_fast9.c \
					c_filter.c \
					c_histogram.c \
					c_integralimage.c \
					c_lut.c \
					c_magnitude.c \
					c_sobel3x3.c \
					c_statistics.c 
LOCAL_C_INCLUDES := $(OPENVX_INC) $(OPENVX_TOP)/$(OPENVX_SRC)/include $(OPENVX_TOP)/debug
LOCAL_MODULE := libopenvx-c_model-lib
include $(BUILD_STATIC_LIBRARY)