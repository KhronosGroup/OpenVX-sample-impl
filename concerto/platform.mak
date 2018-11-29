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

ifeq ($(TARGET_OS),LINUX)
    PLATFORM_LIBS := dl pthread rt
else ifeq ($(TARGET_OS),DARWIN)
    PLATFORM_LIBS :=
else ifeq ($(TARGET_OS),Windows_NT)
    PLATFORM_LIBS := Ws2_32 user32 kernel32 Gdi32
else ifeq ($(TARGET_OS),__QNX__)
    PLATFORM_LIBS := screen socket
else ifeq ($(TARGET_OS),CYGWIN)
    PLATFORM_LIBS := c pthread
endif

