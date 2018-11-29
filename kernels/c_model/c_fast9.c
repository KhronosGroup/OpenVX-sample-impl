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
#include <stdio.h>
#include <string.h>

#define PERMUTATIONS 16
#define APERTURE 3

static vx_uint8 indexes[PERMUTATIONS][9] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8},
    {15, 0, 1, 2, 3, 4, 5, 6, 7},
    {14,15, 0, 1, 2, 3, 4, 5, 6},
    {13,14,15, 0, 1, 2, 3, 4, 5},
    {12,13,14,15, 0, 1, 2, 3, 4},
    {11,12,13,14,15, 0, 1, 2, 3},
    {10,11,12,13,14,15, 0, 1, 2},
    { 9,10,11,12,13,14,15, 0, 1},
    { 8, 9,10,11,12,13,14,15, 0},
    { 7, 8, 9,10,11,12,13,14,15},
    { 6, 7, 8, 9,10,11,12,13,14},
    { 5, 6, 7, 8, 9,10,11,12,13},
    { 4, 5, 6, 7, 8, 9,10,11,12},
    { 3, 4, 5, 6, 7, 8, 9,10,11},
    { 2, 3, 4, 5, 6, 7, 8, 9,10},
    { 1, 2, 3, 4, 5, 6, 7, 8, 9},
};

/* offsets from "p" */
static vx_int32 offsets[16][2] = {
    {  0, -3},
    {  1, -3},
    {  2, -2},
    {  3, -1},
    {  3,  0},
    {  3,  1},
    {  2,  2},
    {  1,  3},
    {  0,  3},
    { -1,  3},
    { -2,  2},
    { -3,  1},
    { -3,  0},
    { -3, -1},
    { -2, -2},
    { -1, -3},
};


static vx_bool vxIsFastCorner(const vx_uint8* buf, vx_uint8 p, vx_uint8 tolerance)
{
    vx_uint32 i, a;
    for (a = 0; a < PERMUTATIONS; a++)
    {
        vx_bool isacorner = vx_true_e;
        for (i = 0; i < dimof(indexes[a]); i++)
        {
            vx_uint8 j = indexes[a][i];
            vx_uint8 v = buf[j];
            if (v <= (p + tolerance))
            {
                isacorner = vx_false_e;
            }
        }
        if (isacorner == vx_true_e)
            return isacorner;
        isacorner = vx_true_e;
        for (i = 0; i < dimof(indexes[a]); i++)
        {
            vx_uint8 j = indexes[a][i];
            vx_uint8 v = buf[j];
            if (v >= (p - tolerance))
            {
                isacorner = vx_false_e;
            }
        }
        if (isacorner == vx_true_e)
            return isacorner;
    }
    return vx_false_e;
}


static vx_uint8 vxGetFastCornerStrength(vx_int32 x, vx_int32 y, void* src_base,
                                        vx_imagepatch_addressing_t* src_addr,
                                        vx_uint8 tolerance)
{
    if (x < APERTURE || y < APERTURE || x >= (vx_int32)src_addr->dim_x - APERTURE || y >= (vx_int32)src_addr->dim_y - APERTURE)
        return 0;
    {
    vx_uint8 p = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, src_addr);
    vx_uint8 buf[16];
    vx_int32 j;
    vx_uint8 a, b = 255;

    for (j = 0; j < 16; j++)
    {
        buf[j] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+offsets[j][0], y+offsets[j][1], src_addr);
    }

    if (!vxIsFastCorner(buf, p, tolerance))
        return 0;

    a = tolerance;
    while (b - a > 1)
    {
        vx_uint8 c = (a + b)/2;
        if (vxIsFastCorner(buf, p, c))
            a = c;
        else
            b = c;
    }
    return a;
    }
}

// nodeless version of the Fast9Corners kernel
vx_status vxFast9Corners(vx_image src, vx_scalar sens, vx_scalar nonm, vx_array points,
                         vx_scalar s_num_corners, vx_border_t *bordermode)
{
    vx_float32 b = 0.0f;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    void *src_base = NULL;
    vx_rectangle_t rect;
    vx_uint8 tolerance = 0;
    vx_bool do_nonmax;
    vx_size num_corners = 0;
    vx_size dst_capacity = 0;
    vx_map_id map_id = 0;
    vx_keypoint_t kp;

    vx_status status = vxGetValidRegionImage(src, &rect);

    status |= vxCopyScalar(sens, &b, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(nonm, &do_nonmax, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    /* remove any pre-existing points */
    status |= vxTruncateArray(points, 0);
    status |= vxMapImagePatch(src, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

    tolerance = (vx_uint8)b;

    status |= vxQueryArray(points, VX_ARRAY_CAPACITY, &dst_capacity, sizeof(dst_capacity));

    memset(&kp, 0, sizeof(kp));

    if (status == VX_SUCCESS)
    {
        /*! \todo implement other Fast9 Corners border modes */
        if (bordermode->mode == VX_BORDER_UNDEFINED)
        {
            vx_int32 y, x;
            for (y = APERTURE; y < (vx_int32)(src_addr.dim_y - APERTURE); y++)
            {
                for (x = APERTURE; x < (vx_int32)(src_addr.dim_x - APERTURE); x++)
                {
                    vx_uint8 strength = vxGetFastCornerStrength(x, y, src_base, &src_addr, tolerance);
                    if (strength > 0)
                    {
                        if (do_nonmax)
                        {
                            if (strength >= vxGetFastCornerStrength(x-1, y-1, src_base, &src_addr, tolerance) &&
                                strength >= vxGetFastCornerStrength(x, y-1, src_base, &src_addr, tolerance) &&
                                strength >= vxGetFastCornerStrength(x+1, y-1, src_base, &src_addr, tolerance) &&
                                strength >= vxGetFastCornerStrength(x-1, y, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x+1, y, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x-1, y+1, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x, y+1, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x+1, y+1, src_base, &src_addr, tolerance))
                                ;
                            else
                                continue;
                        }
                        if (num_corners < dst_capacity)
                        {
                            kp.x = x;
                            kp.y = y;
                            kp.strength = strength;
                            kp.scale = 0.0f;
                            kp.orientation = 0.0f;
                            kp.tracking_status = 1;
                            kp.error = 0.0f;
                            status |= vxAddArrayItems(points, 1, &kp, sizeof(kp));
                        }
                        num_corners++;
                    }
                }
            }
        }
        else
        {
            status = VX_ERROR_NOT_IMPLEMENTED;
        }

        if (s_num_corners)
            status |= vxCopyScalar(s_num_corners, &num_corners, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

        status |= vxUnmapImagePatch(src, map_id);
    }

    return status;
}

