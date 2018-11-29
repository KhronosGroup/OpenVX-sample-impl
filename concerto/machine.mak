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

define MACHINE_variables
ifneq ($(filter $($(1)_CPU),x86 X86 i386 i486 i586 i686),)
    HOST_PLATFORM=PC
    $(1)_FAMILY=X86
    $(1)_ARCH=32
    $(1)_ENDIAN=LITTLE
else ifneq ($(filter $($(1)_CPU),Intel64 amd64 X64),)
    HOST_PLATFORM=PC
    $(1)_FAMILY=X64
    $(1)_ARCH=64
    $(1)_ENDIAN=LITTLE
else ifeq ($($(1)__CPU),Power Macintosh)
    HOST_PLATFORM=PC
    $(1)_FAMILY=PPC
    $(1)_ARCH=32
    $(1)_ENDIAN=LITTLE
else ifeq ($($(1)_CPU),x86_64)
    HOST_PLATFORM=PC
    $(1)_FAMILY=x86_64
    $(1)_ARCH=64
    $(1)_ENDIAN=LITTLE
else ifneq ($(filter $($(1)_CPU),ARM M3 M4 A8 A8F A9 A9F A15 A15F armv7l),)
    ifeq ($(HOST_CPU),$($(1)_CPU))
        HOST_PLATFORM=PANDA
    else
        HOST_PLATFORM=PC
    endif
    $(1)_FAMILY=ARM
    $(1)_ARCH=32
    $(1)_ENDIAN=LITTLE
else ifneq ($(filter $($(1)_CPU),ARM64 aarch64 A53 A54 A57),)
    $(1)_FAMILY=ARM
    $(1)_ARCH=64
    $(1)_ENDIAN=LITTLE
else ifneq ($(filter $($(1)_CPU),C6XSIM C64T C64P C64 C66 C674 C67 C67P),)
    HOST_PLATFORM=PC
    $(1)_FAMILY=DSP
    $(1)_ARCH=32
    $(1)_ENDIAN=LITTLE
else ifeq ($($(1)_CPU),EVE)
    HOST_PLATFORM=PC
    $(1)_FAMILY=EVE
    $(1)_ARCH=32
    $(1)_ENDIAN=LITTLE
endif
endef


ifeq ($(HOST_OS),Windows_NT)
    $(info Windows Processor Architecture $(PROCESSOR_ARCHITECTURE))
    $(info Windows Processor Identification $(word 1, $(PROCESSOR_IDENTIFIER)))
    HOST_CPU=$(word 1, $(PROCESSOR_IDENTIFIER))
else
    HOST_CPU=$(shell uname -m)
endif

$(eval $(call MACHINE_variables,HOST))

