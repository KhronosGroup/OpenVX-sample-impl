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

#ifndef _VX_INLINES_C_
#define _VX_INLINES_C_

#include <VX/vx.h>
#include "vx_internal.h"

static VX_INLINE void *ownFormatMemoryPtr(vx_memory_t *memory,
                                          vx_uint32 c,
                                          vx_uint32 x,
                                          vx_uint32 y,
                                          vx_uint32 p)
{
    intmax_t offset = (memory->strides[p][VX_DIM_Y] * y) +
                      (memory->strides[p][VX_DIM_X] * x) +
                      (memory->strides[p][VX_DIM_C] * c);
    void *ptr = (void *)&memory->ptrs[p][offset];
    //ownPrintMemory(memory);
    //VX_PRINT(VX_ZONE_INFO, "&(%p[%zu]) = %p\n", memory->ptrs[p], offset, ptr);
    return ptr;
}

#endif
