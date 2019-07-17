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


_MODULE     := openvx-c_model-lib
include $(PRELUDE)
TARGET      := openvx-c_model-lib
TARGETTYPE  := library
CSOURCES    := $(call all-c-files)
IDIRS       := $(HOST_ROOT)/debug $(HOST_ROOT)/utils
SHARED_LIBS := openvx
include $(FINALE)

