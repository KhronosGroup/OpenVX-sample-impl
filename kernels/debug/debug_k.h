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

#ifndef _VX_DEBUG_K_H_
#define _VX_DEBUG_K_H_

#include <VX/vx.h>
#include <VX/vx_helper.h>

#define FGETS(str, fh)                              \
{                                                   \
    char* success = fgets(str, sizeof(str), fh);    \
    if (!success)                                   \
    {                                               \
        printf("fgets failed\n");                   \
    }                                               \
}

#ifdef __cplusplus
extern "C" {
#endif

vx_status ownFWriteImage (vx_image input, vx_array filename);
vx_status ownFReadImage  (vx_array filename, vx_image output);

vx_status ownCopyImage(vx_image input, vx_image output);
vx_status ownCopyArray(vx_array src, vx_array dst);

#ifdef __cplusplus
}
#endif

#endif  // !_VX_DEBUG_K_H_

