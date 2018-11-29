#

# Copyright (c) 2012-2018 The Khronos Group Inc.
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


CONCERTO_ROOT ?= concerto
DIRECTORIES := include examples helper debug libraries kernels sample api-docs tools cts utils
OPENVX_SRC := sample
BUILD_TARGET ?= $(CONCERTO_ROOT)/target.mak
# By not setting target combos it will automatically generate them.
#TARGET_COMBOS:=PC:LINUX:x86_64:0:debug:GCC
#TARGET_COMBOS+=PC:LINUX:x86_64:0:release:GCC
#TARGET_COMBOS+=PC:Windows_NT:X64:0:release:CL
#TARGET_COMBOS+=PC:Windows_NT:X64:0:debug:CL
#TARGET_COMBOS+=PC:LINUX:ARM:0:debug:RVCT
include $(CONCERTO_ROOT)/rules.mak


#==========================
# All packages
#==========================

packages: openvx_conformance_package \
          openvx_sample_package \
          openvx-header-packages

#==========================
# Conformance test package
#==========================

# Make a bzip2 tarball containing just the conformance tests.
OPENVX_CONFORMANCE_DIR = cts

# This is just the default destination, expected to be overridden.
OPENVX_CONFORMANCE_DESTDIR = $(HOST_ROOT)/out

OPENVX_CONFORMANCE_PACKAGE_NAME = \
 OpenVX-CTS-$(VERSION)-$(strip $(shell date '+%Y%m%d')).tar.bz2

# Only do this for the same host environments where the concerto
# "tar" package type is available.
ifneq ($(filter $(HOST_OS),LINUX CYGWIN DARWIN),)

.PHONY: openvx_conformance_package
openvx_conformance_package: \
 $(OPENVX_CONFORMANCE_DESTDIR)/$(OPENVX_CONFORMANCE_PACKAGE_NAME)

$(OPENVX_CONFORMANCE_DESTDIR)/$(OPENVX_CONFORMANCE_PACKAGE_NAME): \
 $(patsubst %, $(HOST_ROOT)/%, $(OPENVX_CONFORMANCE_DIRS))
	@$(MKDIR) $(OPENVX_CONFORMANCE_DESTDIR)
	@tar --exclude=.svn --exclude=CMakeLists.txt.user -cjf $@ \
	$(OPENVX_CONFORMANCE_DIR)

endif # Host environments.
