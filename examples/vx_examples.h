/* 

 * Copyright (c) 2012-2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file vx_examples.h
 * \brief A header of example functions.
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_example OpenVX Examples
 * \brief A series of examples of how to use the OpenVX API.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>

int example_pipeline(int argc, char *argv[]);
int example_corners(int argc, char *argv[]);
int example_ar(int argc, char *argv[]);
int example_super_resolution(int argc, char *argv[]);
