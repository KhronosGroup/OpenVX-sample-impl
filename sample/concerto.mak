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


# Make a bzip2 tarball containing just the sample implementation.

OPENVX_SAMPLE_DIR_NAMES = \
 cmake_utils concerto debug examples helper include kernels libraries raw sample scripts tools utils \
 Android.mk BUILD_DEFINES Build.py CMakeLists.txt LICENSE Makefile NEWKERNEL.txt README VERSION

OPENVX_SAMPLE_DIRS = $(patsubst %, %, $(OPENVX_SAMPLE_DIR_NAMES))

# This is just the default destination, expected to be overridden.
OPENVX_SAMPLE_DESTDIR = $(HOST_ROOT)/out

OPENVX_SAMPLE_PACKAGE_NAME = \
 openvx_sample_$(VERSION)-$(strip $(shell date '+%Y%m%d')).tar.bz2
OPENVX_SAMPLE_PACKAGE_TOPDIR_NAME = openvx_sample

# For OPENVX_SAMPLE_PACKAGE_TOPDIR_NAME to work, we need
# someplace to put a symbolic link when packaging, as tar does
# not have an option to set a "virtual" top directory name.
OPENVX_SAMPLE_TMPDIR = $(HOST_ROOT)/out/packagetmp

# Only do this for the same host environments where the concerto
# "tar" package type is available.
ifneq ($(filter $(HOST_OS),LINUX CYGWIN DARWIN),)

.PHONY: openvx_sample_package
openvx_sample_package: \
 $(OPENVX_SAMPLE_DESTDIR)/$(OPENVX_SAMPLE_PACKAGE_NAME)

# The exclusion of .svn directories is there for the benefit of
# svn clients before 1.7, where there's a .svn directory in
# every directory in the working copy.
$(OPENVX_SAMPLE_DESTDIR)/$(OPENVX_SAMPLE_PACKAGE_NAME): \
 $(patsubst %, $(HOST_ROOT)/%, $(OPENVX_SAMPLE_DIRS))
	@$(MKDIR) $(OPENVX_SAMPLE_DESTDIR)
	@$(MKDIR) $(OPENVX_SAMPLE_TMPDIR)
	@$(LINK) $(HOST_ROOT) $(OPENVX_SAMPLE_TMPDIR)/$(OPENVX_SAMPLE_PACKAGE_TOPDIR_NAME)
	@tar --exclude=.svn -cjf $@ -C $(OPENVX_SAMPLE_TMPDIR) \
	 $(patsubst %, $(OPENVX_SAMPLE_PACKAGE_TOPDIR_NAME)/%, $(OPENVX_SAMPLE_DIRS))
	@$(CLEANDIR) $(OPENVX_SAMPLE_TMPDIR)

endif # Host environments.
