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

ifeq ($(HOST_OS),Windows_NT) # cmd.exe
CLEAN    := del /Q
CLEANDIR := rmdir /Q /S
COPY     := copy /Y /Z /V
PRINT    := @echo
SET_RW   := attrib -R
SET_EXEC := echo
LINK     := junction
TOUCH    := type NUL >
INSTALL  := copy /Y /Z /V
MKDIR    := mkdir
CAT      := type
QUIET 	 := 2>NUL
else # Bash variants
CLEAN    := rm -f
CLEANDIR := rm -rf
COPY     := cp -f
PRINT    := @echo
SET_RW   := chmod a+rw
SET_EXEC := chmod a+x
LINK     := ln -s -f
TOUCH    := touch
INSTALL  := install -C -m 755
MKDIR    := mkdir -p
CAT      := cat
QUIET    := > /dev/null
endif

