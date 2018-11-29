#
# Copyright (c) 2017-2018 The Khronos Group Inc.
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

# Get headers files for the IDE
file( GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h )
file( GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c )

set( LIST_OF_SAMPLE_HEADER_FILES ${HEADER_FILES} ${SOURCE_FILES} PARENT_SCOPE )
set( SAMPLE_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
