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

ifeq ($(USE_OPENCL),true)
    OCL_LIB ?= OpenCL
    ifeq ($(HOST_OS),Windows_NT)
        ifeq ($(OPENCL_ROOT),)
            $(error OPENCL_ROOT must be defined to use OPENCL_ROOT)
        endif
        IDIRS += $(OPENCL_ROOT)/include $(OPENCL_ROOT)/inc
        LDIRS += $(OPENCL_ROOT)/lib $(OPENCL_ROOT)/lib64
        ifeq ($(filter $(PLATFORM_LIBS),$(OCL_LIB)),)
            PLATFORM_LIBS += $(OCL_LIB)
        endif
    else ifeq ($(HOST_OS),LINUX)
        # User should install GLUT/Mesa via package system
        ifneq ($(OPENCL_ROOT),)
            IDIRS += $(OPENCL_ROOT)/include $(OPENCL_ROOT)/inc
            LDIRS += $(OPENCL_ROOT)/lib $(OPENCL_ROOT)/lib64
        endif
        ifeq ($(filter $(PLATFORM_LIBS),$(OCL_LIB)),)
            PLATFORM_LIBS += $(OCL_LIB)
        endif
    else ifeq ($(HOST_OS),DARWIN)
        # User should have XCode install OpenCL
        $(_MODULE)_FRAMEWORKS += -framework OpenCL
    endif
    
    # OpenCL-Environment Defines
    ifeq ($(HOST_OS),CYGWIN)
        DEFS += KDIR="\"$(KDIR)\"" CL_USER_DEVICE_COUNT=$(CL_USER_DEVICE_COUNT) CL_USER_DEVICE_TYPE="\"$(CL_USER_DEVICE_TYPE)\""
    else ifeq ($(HOST_OS),Windows_NT)
        DEFS += KDIR="$(call PATH_CONV,$(KDIR))\\" CL_USER_DEVICE_COUNT=$(CL_USER_DEVICE_COUNT) CL_USER_DEVICE_TYPE="$(CL_USER_DEVICE_TYPE)"
    else
        DEFS += KDIR="$(KDIR)" CL_USER_DEVICE_COUNT=$(CL_USER_DEVICE_COUNT) CL_USER_DEVICE_TYPE="$(CL_USER_DEVICE_TYPE)" $(if $(CL_DEBUG),CL_DEBUG=1)
    endif
endif
