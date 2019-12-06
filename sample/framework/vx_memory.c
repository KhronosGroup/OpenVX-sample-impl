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

#include "vx_internal.h"
#include "vx_memory.h"

vx_bool ownFreeMemory(vx_context context, vx_memory_t *memory)
{
    (void)context;

    if (memory->allocated == vx_true_e)
    {
        vx_uint32 p = 0u;
        ownPrintMemory(memory);
        for (p = 0; p < memory->nptrs; p++)
        {
            if (memory->ptrs[p])
            {
                VX_PRINT(VX_ZONE_INFO, "Freeing %p\n", memory->ptrs[p]);
                free(memory->ptrs[p]);
                ownDestroySem(&memory->locks[p]);
                memory->ptrs[p] = NULL;
            }
        }
        memory->allocated = vx_false_e;
    }
    return memory->allocated;
}


vx_bool ownAllocateMemory(vx_context context, vx_memory_t *memory)
{
    (void)context;

    if (memory->allocated == vx_false_e)
    {
        vx_uint32 d = 0;
        vx_uint32 p = 0;
        VX_PRINT(VX_ZONE_INFO, "Allocating %u pointers of %u dimensions each.\n", memory->nptrs, memory->ndims);
        memory->allocated = vx_true_e;
        for (p = 0; p < memory->nptrs; p++)
        {
            vx_size size = sizeof(vx_uint8);
            /* channel is a declared size, don't assume */
            if (memory->strides[p][VX_DIM_C] != 0)
                size = (size_t)abs(memory->strides[p][VX_DIM_C]);
            else if (memory->stride_x_bits[p] != 0)
            {
                /* data type is not whole number of bytes */
                size = 0ul;
            }
            if (size == 0ul)
            {
                memory->strides[p][VX_DIM_X] = 0;
                /* the size of each row in the x-dimension, in integer number of bytes (rounded up from bits) */
                size = ((size_t)abs(memory->stride_x_bits[p]) * (vx_size)memory->dims[p][VX_DIM_X] + 7ul) / 8ul;
                for (d = 2; d < memory->ndims; d++)
                {
                    memory->strides[p][d] = (vx_int32)size;
                    size *= (vx_size)memory->dims[p][d];
                }
            }
            else
            {
                /* default behavior */
                for (d = 0; d < memory->ndims; d++)
                {
                    memory->strides[p][d] = (vx_int32)size;
                    size *= (vx_size)memory->dims[p][d];
                }
            }
            /* don't presume that memory should be zeroed */
            memory->ptrs[p] = malloc(size);
            if (memory->ptrs[p] == NULL)
            {
                vx_uint32 pi;
                VX_PRINT(VX_ZONE_ERROR, "Failed to allocated "VX_FMT_SIZE" bytes\n", size);
                /* unroll */
                memory->allocated = vx_false_e;
                for (pi = 0; pi < p; pi++)
                {
                    VX_PRINT(VX_ZONE_INFO, "Freeing %p\n", memory->ptrs[pi]);
                    free(memory->ptrs[pi]);
                    memory->ptrs[pi] = NULL;
                }
                break;
            }
            else
            {
                ownCreateSem(&memory->locks[p], 1);
                VX_PRINT(VX_ZONE_INFO, "Allocated %p for "VX_FMT_SIZE" bytes\n", memory->ptrs[p], size);
            }
        }
        ownPrintMemory(memory);
    }
    return memory->allocated;
}

void ownPrintMemory(vx_memory_t *mem)
{
    vx_uint32 d = 0;
    vx_uint32 p = 0;
    for (p = 0; p < mem->nptrs; p++)
    {
        vx_bool gotlock = ownSemTryWait(&mem->locks[p]);
        if (gotlock == vx_true_e)
            ownSemPost(&mem->locks[p]);
        VX_PRINT(VX_ZONE_INFO, "ptr[%u]=%p %s stride_x_bits[%u]=%u\n", p, mem->ptrs[p],
                (gotlock==vx_true_e?"UNLOCKED":"LOCKED"), p, mem->stride_x_bits[p]);
        for (d = 0; d < mem->ndims; d++)
        {
            VX_PRINT(VX_ZONE_INFO, "\tdim[%u][%u]=%d strides[%u][%u]=%d\n", p, d, mem->dims[p][d], p, d, mem->strides[p][d]);
        }
    }
}

vx_size ownComputeMemorySize(vx_memory_t *memory, vx_uint32 p)
{
    return (memory->ndims == 0 ? 0 : (memory->dims[p][memory->ndims-1] * memory->strides[p][memory->ndims-1]));
}

