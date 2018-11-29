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

ifeq ($(SCM_ROOT),)
# wildcard is required for windows since realpath doesn't return NULL if not found
SCM_ROOT := $(wildcard $(realpath .svn))
ifneq ($(SCM_ROOT),)
ifeq ($(SHOW_MAKEDEBUG),1)
$(info $(SCM_ROOT))
$(info Subversion is used)
endif
SCM_VERSION := r$(word 2, $(shell svn info | grep Revision))
endif
endif

ifeq ($(SCM_ROOT),)
# wildcard is required for windows since realpath doesn't return NULL if not found
SCM_ROOT := $(wildcard $(realpath .git))
ifneq ($(SCM_ROOT),)
ifeq ($(SHOW_MAKEDEBUG),1)
$(info $(SCM_ROOT))
$(info GIT is used)
endif
git_status := $(shell git status)
ifneq (,$(findstring Changes,$(git_status)))
    SCM_VERSION := $(shell git rev-parse --short HEAD)-dirty
else
    SCM_VERSION := $(shell git rev-parse --short HEAD)
endif
# The following is in case git is not executable in the shell where the build is taking place (eg. DOS)
ifeq ($(SCM_VERSION),)
TEMP := $(lastword $(shell $(CAT) $(call PATH_CONV,"$(SCM_ROOT)/HEAD")))
SCM_VERSION := $(shell $(CAT) $(call PATH_CONV,"$(SCM_ROOT)/$(TEMP)"))
endif
endif
endif

