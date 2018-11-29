#

# Copyright (c) 2015-2017 The Khronos Group Inc.
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


# Make a bzip2 tarball containing just the standard headers
# (no extensions).

OPENVX_STD_HEADER_BASENAMES = \
 vx.h vxu.h vx_vendors.h vx_types.h vx_kernels.h vx_api.h vx_nodes.h vx_compatibility.h
OPENVX_EXT_HEADER_BASENAMES = \
 vx_import.h vx_khr_class.h vx_khr_icd.h vx_khr_ix.h vx_khr_nn.h vx_khr_tiling.h vx_khr_xml.h

OPENVX_STD_HEADERS = $(patsubst %, VX/%, $(OPENVX_STD_HEADER_BASENAMES))
OPENVX_EXT_HEADERS = $(patsubst %, VX/%, $(OPENVX_EXT_HEADER_BASENAMES))

# This is just the default destination, expected to be overridden.
OPENVX_HEADERS_DESTDIR = $(HOST_ROOT)/out

OPENVX_STD_HEADERS_PACKAGE_NAME = \
 openvx-standard-headers-$(VERSION)-$(strip $(shell date '+%Y%m%d')).tar.bz2
OPENVX_EXT_HEADERS_PACKAGE_NAME = \
 openvx-extension-headers-$(VERSION)-$(strip $(shell date '+%Y%m%d')).tar.bz2

# Only do this for the same host environments where the concerto
# "tar" package type is available.
ifneq ($(filter $(HOST_OS),LINUX CYGWIN DARWIN),)

.PHONY: openvx-header-packages
openvx-header-packages: \
 $(OPENVX_HEADERS_DESTDIR)/$(OPENVX_STD_HEADERS_PACKAGE_NAME) \
 $(OPENVX_HEADERS_DESTDIR)/$(OPENVX_EXT_HEADERS_PACKAGE_NAME)

$(OPENVX_HEADERS_DESTDIR)/$(OPENVX_STD_HEADERS_PACKAGE_NAME): \
 $(patsubst %, $(HOST_ROOT)/include/%, $(OPENVX_STD_HEADERS))
	@$(MKDIR) $(OPENVX_HEADERS_DESTDIR)
	@tar -cjf $@ -C $(HOST_ROOT)/include $(OPENVX_STD_HEADERS)

$(OPENVX_HEADERS_DESTDIR)/$(OPENVX_EXT_HEADERS_PACKAGE_NAME): \
 $(patsubst %, $(HOST_ROOT)/include/%, $(OPENVX_EXT_HEADERS))
	@$(MKDIR) $(OPENVX_HEADERS_DESTDIR)
	@tar -cjf $@ -C $(HOST_ROOT)/include $(OPENVX_EXT_HEADERS)

endif # Host environments.
