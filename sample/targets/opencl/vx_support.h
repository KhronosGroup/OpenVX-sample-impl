/* 

 * Copyright (c) 2011-2017 The Khronos Group Inc.
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

#include <vx_interface.h>
#include "vx_internal.h"

#if defined(__ANDROID__) || defined(__linux__) || defined(__QNX__) || defined(__CYGWIN__) || defined(__APPLE__)
#if !defined(__QNX__) && !defined(__APPLE__)
#include <features.h>
#else
#define __EXT_UNIX_MISC //Needed by QNX version of dirent.h to include scandir()
#endif
#include <sys/types.h>
#if defined(__APPLE__)
#include <sys/dirent.h>
#endif
#include <dirent.h>
#include <fnmatch.h>
#define EXPERIMENTAL_USE_FNMATCH
#elif defined(_WIN32)
#define snprintf _snprintf
#endif

#define CL_MAX_LINESIZE (1024)

#define ALLOC(type,count)                               (type *)calloc(count, sizeof(type))
#define CL_ERROR_MSG(err, string)                       clPrintError(err, string, __FUNCTION__, __FILE__, __LINE__)
#define CL_BUILD_MSG(err, string)                       clBuildError(err, string, __FUNCTION__, __FILE__, __LINE__)

char *clLoadSources(char *filename, size_t *programSize);
cl_int clBuildError(cl_int build_status, const char *label, const char *function, const char *file, int line);
cl_int clPrintError(cl_int err, const char *label, const char *function, const char *file, int line);
