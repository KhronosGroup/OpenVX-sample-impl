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
#include <stdlib.h>

vx_uint8 vx_lbp_s(vx_int16 x)
{
    if(x >= 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

vx_uint8 vx_lbp_u(vx_uint8 *g, vx_uint8 gc)
{
    vx_uint8 u1 = vx_lbp_s(g[7] - gc);
    vx_uint8 u2 = vx_lbp_s(g[0] - gc);

    vx_uint8 abs1 = abs(u1 - u2);

    vx_uint8 abs2 = 0;
    for(vx_int8 p = 1; p < 8; p++)
    {
        u1 = vx_lbp_s(g[p] - gc);
        u2 = vx_lbp_s(g[p-1] - gc);
        abs2 += abs(u1 - u2);
    }

    return abs1 + abs2;
}

vx_status vxLBPStandard(vx_image src, vx_int8 ksize, vx_image dst)
{
    vx_rectangle_t rect;
    vx_uint32 y = 0, x = 0;
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint8 gc, g[8], sum;

    status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    if( status != VX_SUCCESS )
    {
        return status;
    }

    if(ksize == 3)
    {
        for (y = 1; y < src_addr.dim_y - 1; y += src_addr.step_y)
        {
            for (x = 1; x < src_addr.dim_x - 1; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-1,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+1,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                for(vx_int8 p = 0; p < 8; p++)
                {
                    sum += vx_lbp_s(g[p] - gc) * (1 << p);
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }
     else if (ksize == 5)
     {
        for (y = 2; y < src_addr.dim_y - 2; y += src_addr.step_y)
        {
            for (x = 2; x < src_addr.dim_x - 2; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-2,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+2,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                for(vx_int8 p = 0; p < 8; p++)
                {
                    sum += vx_lbp_s(g[p] - gc) * (1 << p);
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }

     status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
     status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
     return status;
}

vx_status vxLBPModified(vx_image src, vx_image dst)
{
    vx_rectangle_t rect;
    vx_uint32 y = 0, x = 0;
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint8 avg, g[8], sum;

    status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    if( status != VX_SUCCESS )
    {
        return status;
    }

    for (y = 2; y < src_addr.dim_y - 2; y += src_addr.step_y)
    {
        for (x = 2; x < src_addr.dim_x - 2; x += src_addr.step_x)
        {
            g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y-2,  &src_addr);
            g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-2,  &src_addr);
            g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y-2,  &src_addr);
            g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y,    &src_addr);
            g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y+2,  &src_addr);
            g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+2,  &src_addr);
            g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y+2,  &src_addr);
            g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y,    &src_addr);

            avg =(g[0] + g[1] + g[2] + g[3] + g[4] + g[5] + g[6] + g[7] + 1)/8;

            sum = 0;
            for(vx_int8 p = 0; p < 8; p++)
            {
                sum += ((g[p] > avg) * (1 << p));
            }

            vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            *dst_ptr = sum;
        }
     }

     status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
     status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

     return status;
}

vx_status vxLBPUniform(vx_image src, vx_int8 ksize, vx_image dst)
{
    vx_rectangle_t rect;
    vx_uint32 y = 0, x = 0;
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint8 gc, g[8], sum;

    status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    if( status != VX_SUCCESS )
    {
        return status;
    }

    if(ksize == 3)
    {
        for (y = 1; y < src_addr.dim_y - 1; y += src_addr.step_y)
        {
            for (x = 1; x < src_addr.dim_x - 1; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-1,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+1,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                if(vx_lbp_u(g, gc) <= 2)
                {
                    for (vx_uint8 p = 0; p < 8; p++)
                    {
                        sum += vx_lbp_s(g[p] - gc)*(1 << p);
                    }
                }
                else
                {
                    sum = 9;
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }
     else if (ksize == 5)
     {
        for (y = 2; y < src_addr.dim_y - 2; y += src_addr.step_y)
        {
            for (x = 2; x < src_addr.dim_x - 2; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-2,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+2,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                if(vx_lbp_u(g, gc) <= 2)
                {
                    for (vx_uint8 p = 0; p < 8; p++)
                    {
                        sum += vx_lbp_s(g[p] - gc)*(1 << p);
                    }
                }
                else
                {
                    sum = 9;
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }

     status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
     status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
     return status;
}

// nodeless version of the LBP kernel
vx_status vxLBP(vx_image src, vx_scalar sformat, vx_scalar ksize, vx_image dst)
{
    vx_status status = VX_FAILURE;

    vx_enum format;
    vx_int8 size;
    vxCopyScalar(sformat, &format, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyScalar(ksize, &size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    switch (format)
    {
        case VX_LBP:
            status = vxLBPStandard(src, size, dst);
            break;
        case VX_MLBP:
            status = vxLBPModified(src, dst);
            break;
        case VX_ULBP:
            status = vxLBPUniform(src, size, dst);
            break;
    }
    return status;
}

