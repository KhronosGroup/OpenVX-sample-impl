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

#include <c_model.h>
#include "tensor_utils.h"


// nodeless version of the TableLookup kernel
vx_status vxTensorTableLookup(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims, void* lut, vx_size lut_size,
        vx_uint32 lut_offset, void* dst, vx_size* dst_strides, vx_enum type)
{
    vx_status status = VX_SUCCESS;
    vx_size out_size = ComputeNumberOfElements(dims, num_of_dims);
    for (vx_size i = 0; i < out_size; i++)
    {
        vx_size src_pos0 = 0, dst_pos0 = 0;
        vx_size src_pos = ComputeGlobalPositionsFromIndex(i, dims, src_strides, num_of_dims, &src_pos0);
        vx_size dst_pos = ComputeGlobalPositionsFromIndex(i, dims, dst_strides, num_of_dims, &dst_pos0);
        if (type == VX_TYPE_UINT8)
        {
            vx_uint8 *src_ptr = (vx_uint8*)src+src_pos;
            vx_uint8 *dst_ptr = (vx_uint8*)dst+dst_pos;
            vx_uint8 *lut_tmp = (vx_uint8*)lut;
            vx_int32 index = (vx_int32)lut_offset + (vx_int32)(*src_ptr);
            if (index >= 0 && index < (vx_int32)lut_size)
            {
                *dst_ptr = lut_tmp[index];
            }
        }
        else if (type == VX_TYPE_INT16)
        {
            vx_int16 *src_ptr = (vx_int16*)((vx_uint8*)src+src_pos);
            vx_int16 *dst_ptr = (vx_int16*)((vx_uint8*)dst+dst_pos);
            vx_int16 *lut_tmp = (vx_int16 *)lut;
            vx_int32 index = (vx_int32)lut_offset + (vx_int32)(*src_ptr);
            if (index >= 0 && index < (vx_int32)lut_size)
            {
                *dst_ptr = lut_tmp[index];
            }
        }
//        else if (type == VX_TYPE_INT8)
//        {
//            vx_int8 *src_ptr = src+src_pos;
//            vx_int8 *dst_ptr = dst+dst_pos;
//            vx_int8 *lut_tmp = (vx_int8 *)lut;
//            vx_int32 index = (vx_int32)lut_offset + (vx_int32)(*src_ptr);
//            if (index >= 0 && index < (vx_int32)lut_size)
//            {
//                *dst_ptr = lut_tmp[index];
//            }
//        }
   }


    return status;
}

