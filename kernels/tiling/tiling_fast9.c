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

#include <arm_neon.h>
#include <tiling.h>
#include <string.h>

#define PERMUTATIONS 16
#define APERTURE 3
#define PERM_SIZE 16

static const vx_uint8 permutations_table[PERMUTATIONS][PERM_SIZE] =
    {
        {  0,  1,  2,  3,  4,  5,  6,  7,  8, 255, 255, 255, 255, 255, 255, 255 },
        { 15,  0,  1,  2,  3,  4,  5,  6,  7, 255, 255, 255, 255, 255, 255, 255 },
        { 14, 15,  0,  1,  2,  3,  4,  5,  6, 255, 255, 255, 255, 255, 255, 255 },
        { 13, 14, 15,  0,  1,  2,  3,  4,  5, 255, 255, 255, 255, 255, 255, 255 },
        { 12, 13, 14, 15,  0,  1,  2,  3,  4, 255, 255, 255, 255, 255, 255, 255 },
        { 11, 12, 13, 14, 15,  0,  1,  2,  3, 255, 255, 255, 255, 255, 255, 255 },
        { 10, 11, 12, 13, 14, 15,  0,  1,  2, 255, 255, 255, 255, 255, 255, 255 },
        {  9, 10, 11, 12, 13, 14, 15,  0,  1, 255, 255, 255, 255, 255, 255, 255 },
        {  8,  9, 10, 11, 12, 13, 14, 15,  0, 255, 255, 255, 255, 255, 255, 255 },
        {  7,  8,  9, 10, 11, 12, 13, 14, 15, 255, 255, 255, 255, 255, 255, 255 },
        {  6,  7,  8,  9, 10, 11, 12, 13, 14, 255, 255, 255, 255, 255, 255, 255 },
        {  5,  6,  7,  8,  9, 10, 11, 12, 13, 255, 255, 255, 255, 255, 255, 255 },
        {  4,  5,  6,  7,  8,  9, 10, 11, 12, 255, 255, 255, 255, 255, 255, 255 },
        {  3,  4,  5,  6,  7,  8,  9, 10, 11, 255, 255, 255, 255, 255, 255, 255 },
        {  2,  3,  4,  5,  6,  7,  8,  9, 10, 255, 255, 255, 255, 255, 255, 255 },
        {  1,  2,  3,  4,  5,  6,  7,  8,  9, 255, 255, 255, 255, 255, 255, 255 }
    };

/* The following creates the index registers to retrieve the 16 texels in the Bresenham circle of radius 3 with center in P.
    . . F 0 1 . . .
    . E . . . 2 . .
    D . . . . . 3 .
    C . . P . . 4 .
    B . . . . . 5 .
    . A . . . 6 . .
    . . 9 8 7 . . .
    Where . is an irrelevant texel value
    We want to retrieve all texels [0,F]
    The 4 registers in r will then be used to get these texels out of two tables in the function get_circle_texels()
    The first table holds the top 4 rows of texels
    . . F 0 1 . . .
    . E . . . 2 . .
    D . . . . . 3 .
    C . . P . . 4 .
    The second table the bottom 3 rows of texels
    B . . . . . 5 .
    . A . . . 6 . .
    . . 9 8 7 . . .
*/
static const vx_uint8 top_right[8] =
{
    /* The register r.val[0] will be used to retrieve these texels:
    . . . 0 1 . . .
    . . . . . 2 . .
    . . . . . . 3 .
    . . . . . . 4 .
    */
    3 /* top table, first row, elem 4, value 0 in the diagram above */,
    4 /* top table, first row, elem 5, value 1 in the diagram above */,
    13 /* top table, second row, elem 6, value 2 in the diagram above */,
    22 /* top table, third row, elem 7, value 3 in the diagram above*/,
    30 /* top table, fourth row, elem 7, value 4 in the diagram above*/,
    255,
    255,
    255
};

static const vx_uint8 bottom_right[8] =
{
    /* The register r.val[1] will be used to retrieve these texels:
    . . . . . . 5 .
    . . . . . 6 . .
    . . . . 7 . . .
    */
    255,
    255,
    255,
    255,
    255,
    6 /* low table, first row, elem 7, value 5 in the diagram above*/,
    13 /* low table, second row, elem 6, value 6 in the diagram above*/,
    20 /* low table, third row, elem 5, value 7 in the diagram above*/
};

static const vx_uint8 top_left[8] =
{
    /* The register r.val[2] will be used to retrieve these texels:
    . . F . . . . .
    . E . . . . . .
    D . . . . . . .
    C . . . . . . .
    */
    255,
    255,
    255,
    255,
    24 /* top table, fourth row, elem 1, value C in the diagram above */,
    16 /* top table, third row, elem 1, value D in the diagram above*/,
    9 /* top table, second row, elem 2, value E in the diagram above*/,
    2 /* top table, first row, elem 3, value F in the diagram above*/
};

static const vx_uint8 bottom_left[8] =
{
    /* The register r.val[3] will be used to retrieve these texels:
    B . . . . . . .
    . A . . . . . .
    . . 9 8 . . . .
    */
    19 /* low table, third row, elem 4, value 8 in the diagram above */,
    18 /* low table, third row, elem 3, value 9 in the diagram above */,
    9 /* low table, second row, elem 2, value A in the diagram above */,
    0 /* low table, first row, elem 1, value B in the diagram above */,
    255,
    255,
    255,
    255
};

static void vxAddArrayItems_tiling(vx_tile_array_t *arr, vx_size count, const void *ptr, vx_size stride)
{
    if ((count > 0) && (ptr != NULL) && (stride >= arr->item_size))
    {
        if (arr->num_items + count <= arr->capacity)
        {
            vx_size offset = arr->num_items * arr->item_size;
            vx_uint8 *dst_ptr = (vx_uint8 *)arr->ptr + offset;

            vx_size i;
            for (i = 0; i < count; ++i)
            {
                vx_uint8 *tmp = (vx_uint8 *)ptr;
                memcpy(&dst_ptr[i * arr->item_size], &tmp[i * stride], arr->item_size);
            }

            arr->num_items += count;
        }
    }
}

static void addCorner(vx_int32 y, int16x8_t *pvX, uint8x8_t *pvPred, vx_uint8 *pStrength, vx_size dst_capacity,
                      vx_size *num_corners, vx_tile_array_t *points)
{
    uint8x8_t vPred = *pvPred;
    int16x8_t vX = *pvX;
    vx_keypoint_t kp;
    if (0 != vget_lane_u8(vPred, 0) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 0);
        kp.y = y;
        kp.strength = pStrength[0];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
    if (0 != vget_lane_u8(vPred, 1) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 1);
        kp.y = y;
        kp.strength = pStrength[1];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
    if (0 != vget_lane_u8(vPred, 2) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 2);
        kp.y = y;
        kp.strength = pStrength[2];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
    if (0 != vget_lane_u8(vPred, 3) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 3);
        kp.y = y;
        kp.strength = pStrength[3];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
    if (0 != vget_lane_u8(vPred, 4) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 4);
        kp.y = y;
        kp.strength = pStrength[4];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
    if (0 != vget_lane_u8(vPred, 5) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 5);
        kp.y = y;
        kp.strength = pStrength[5];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
    if (0 != vget_lane_u8(vPred, 6) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 6);
        kp.y = y;
        kp.strength = pStrength[6];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
    if (0 != vget_lane_u8(vPred, 7) && (*num_corners) < dst_capacity)
    {
        kp.x = vgetq_lane_s16(vX, 7);
        kp.y = y;
        kp.strength = pStrength[7];
        kp.scale = 0.0f;
        kp.orientation = 0.0f;
        kp.tracking_status = 1;
        kp.error = 0.0f;
        (void)vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));
        *num_corners += 1;
    }
}

static void getPermIdx(vx_uint32 idx, uint8x8x2_t *pvPermIdx)
{
    uint8x8x2_t vPermIdx = {
        {vld1_u8(permutations_table[idx]), vld1_u8(permutations_table[idx] + 8)}};
    *pvPermIdx = vPermIdx;
}

static void getElemIdx(uint8x8x4_t *pvIdx)
{
    uint8x8x4_t reg = {
            {
                vld1_u8(top_right),
                vld1_u8(bottom_right),
                vld1_u8(top_left),
                vld1_u8(bottom_left)
            }};
    *pvIdx = reg;
}

static vx_uint8 isFastCorner(uint8x8_t *pvVal, vx_uint8 p, vx_uint8 tolerance)
{
    uint8x8x4_t vIdx;
    uint8x8x2_t vPermIdx;
    uint8x8x4_t vTbl_hi = {{pvVal[0], pvVal[1], pvVal[2], pvVal[3]}};
    uint8x8x3_t vTbl_lo = {{pvVal[4], pvVal[5], pvVal[6]}};
    uint8x16_t vPG = vqaddq_u8(vdupq_n_u8(p), vdupq_n_u8(tolerance));
    uint8x16_t vPL = vqsubq_u8(vdupq_n_u8(p), vdupq_n_u8(tolerance));

    getElemIdx(&vIdx);

    uint8x16_t vPixel = vcombine_u8(vtbx3_u8(vtbl4_u8(vTbl_hi, vIdx.val[0]), vTbl_lo, vIdx.val[1]),
                                    vtbx3_u8(vtbl4_u8(vTbl_hi, vIdx.val[2]), vTbl_lo, vIdx.val[3]));
    uint8x8x2_t vTmp = {{vget_low_u8(vPixel), vget_high_u8(vPixel)}};
    uint8x8_t vPermR = vdup_n_u8(0xFF);
    vx_uint8 bPG = 0;
    vx_uint8 bPL = 0;

    for (vx_uint8 idx = 0; idx < PERMUTATIONS; idx++)
    {
        getPermIdx(idx, &vPermIdx);
        uint8x16_t vVal = vcombine_u8(vtbl2_u8(vTmp, vPermIdx.val[0]),
                                      vtbx2_u8(vPermR, vTmp, vPermIdx.val[1]));
        uint8x16_t vPred = vcgtq_u8(vVal, vPG);
        uint64x1_t vRet = vreinterpret_u64_u8(vand_u8(vget_high_u8(vPred), vget_low_u8(vPred)));
        bPG |= (vget_lane_u64(vRet, 0) == UINT64_MAX);

        vPred = vcltq_u8(vVal, vPL);
        uint64x2_t vRet2 = vreinterpretq_u64_u8(vPred);
        bPL |= ((vgetq_lane_u64(vRet2, 0) == UINT64_MAX) && (vgetq_lane_u64(vRet2, 1) == 0xFF));
    }

    return (bPG | bPL);
}

static vx_uint8 getStrength(vx_uint8 bCorner, uint8x8_t *pvVal, vx_uint8 p, vx_uint8 tolerance)
{
    vx_uint8 a = 0, b = 255;

    if (bCorner)
    {
        a = tolerance;
        while (b - a > 1)
        {
            vx_uint8 c = (a + b)/2;
            if (isFastCorner(pvVal, p, c))
                a = c;
            else
                b = c;
        }
    }

    return a;
}

static void fast9CornersPerRow(uint8x8_t *pvPrv, uint8x8_t *pvCur, uint8x8_t *pvNxt, int16x8_t *pvXStep,
                               vx_imagepatch_addressing_t *src_addr, vx_uint8 tolerance, vx_uint8 *pStrength)
{
    vx_uint8 bCorner;
    int16x8_t vX = *pvXStep;
    uint8x8_t vPrv[7], vCur[7], vNxt[7];
    uint8x8_t vTmp[7];

    vx_int32 x;
    for (x = 0; x < 7; x++)
    {
        vPrv[x] = pvPrv[x];
        vCur[x] = pvCur[x];
        vNxt[x] = pvNxt[x];
    }

    if (vgetq_lane_s16(vX, 0) >= APERTURE && vgetq_lane_s16(vX, 0) < (src_addr->dim_x - APERTURE))
    {
        for (vx_uint32 idx = 0; idx < 7; idx++)
        {
            vTmp[idx] = vext_u8(vPrv[idx], vCur[idx], 5);
        }

        bCorner = isFastCorner(vTmp, vget_lane_u8(vCur[3], 0), tolerance);
        pStrength[0] = getStrength(bCorner, vTmp, vget_lane_u8(vCur[3], 0), tolerance);
    }

    if (vgetq_lane_s16(vX, 1) >= APERTURE && vgetq_lane_s16(vX, 1) < (src_addr->dim_x - APERTURE))
    {
        for (vx_uint32 idx = 0; idx < 7; idx++)
        {
            vTmp[idx] = vext_u8(vPrv[idx], vCur[idx], 6);
        }

        bCorner = isFastCorner(vTmp, vget_lane_u8(vCur[3], 1), tolerance);
        pStrength[1] = getStrength(bCorner, vTmp, vget_lane_u8(vCur[3], 1), tolerance);
    }

    if (vgetq_lane_s16(vX, 2) >= APERTURE && vgetq_lane_s16(vX, 2) < (src_addr->dim_x - APERTURE))
    {
        for (vx_uint32 idx = 0; idx < 7; idx++)
        {
            vTmp[idx] = vext_u8(vPrv[idx], vCur[idx], 7);
        }

        bCorner = isFastCorner(vTmp, vget_lane_u8(vCur[3], 2), tolerance);
        pStrength[2] = getStrength(bCorner, vTmp, vget_lane_u8(vCur[3], 2), tolerance);
    }
    if (vgetq_lane_s16(vX, 3) >= APERTURE && vgetq_lane_s16(vX, 3) < (src_addr->dim_x - APERTURE))
    {
        bCorner = isFastCorner(vCur, vget_lane_u8(vCur[3], 3), tolerance);
        pStrength[3] = getStrength(bCorner, vCur, vget_lane_u8(vCur[3], 3), tolerance);
    }
    if (vgetq_lane_s16(vX, 4) >= APERTURE && vgetq_lane_s16(vX, 4) < (src_addr->dim_x - APERTURE))
    {
        for (vx_uint32 idx = 0; idx < 7; idx++)
        {
            vTmp[idx] = vext_u8(vCur[idx], vNxt[idx], 1);
        }

        bCorner = isFastCorner(vTmp, vget_lane_u8(vCur[3], 4), tolerance);
        pStrength[4] = getStrength(bCorner, vTmp, vget_lane_u8(vCur[3], 4), tolerance);
    }
    if (vgetq_lane_s16(vX, 5) >= APERTURE && vgetq_lane_s16(vX, 5) < (src_addr->dim_x - APERTURE))
    {
        for (vx_uint32 idx = 0; idx < 7; idx++)
        {
            vTmp[idx] = vext_u8(vCur[idx], vNxt[idx], 2);
        }

        bCorner = isFastCorner(vTmp, vget_lane_u8(vCur[3], 5), tolerance);
        pStrength[5] = getStrength(bCorner, vTmp, vget_lane_u8(vCur[3], 5), tolerance);
    }
    if (vgetq_lane_s16(vX, 6) >= APERTURE && vgetq_lane_s16(vX, 6) < (src_addr->dim_x - APERTURE))
    {
        for (vx_uint32 idx = 0; idx < 7; idx++)
        {
            vTmp[idx] = vext_u8(vCur[idx], vNxt[idx], 3);
        }

        bCorner = isFastCorner(vTmp, vget_lane_u8(vCur[3], 6), tolerance);
        pStrength[6] = getStrength(bCorner, vTmp, vget_lane_u8(vCur[3], 6), tolerance);
    }
    if (vgetq_lane_s16(vX, 7) >= APERTURE && vgetq_lane_s16(vX, 7) < (src_addr->dim_x - APERTURE))
    {
        for (vx_uint32 idx = 0; idx < 7; idx++)
        {
            vTmp[idx] = vext_u8(vCur[idx], vNxt[idx], 4);
        }

        bCorner = isFastCorner(vTmp, vget_lane_u8(vCur[3], 7), tolerance);
        pStrength[7] = getStrength(bCorner, vTmp, vget_lane_u8(vCur[3], 7), tolerance);
    }
}

static vx_uint8 indexes[PERMUTATIONS][9] = 
{
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },
    { 15, 0, 1, 2, 3, 4, 5, 6, 7 },
    { 14,15, 0, 1, 2, 3, 4, 5, 6 },
    { 13,14,15, 0, 1, 2, 3, 4, 5 },
    { 12,13,14,15, 0, 1, 2, 3, 4 },
    { 11,12,13,14,15, 0, 1, 2, 3 },
    { 10,11,12,13,14,15, 0, 1, 2 },
    { 9,10,11,12,13,14,15, 0, 1 },
    { 8, 9,10,11,12,13,14,15, 0 },
    { 7, 8, 9,10,11,12,13,14,15 },
    { 6, 7, 8, 9,10,11,12,13,14 },
    { 5, 6, 7, 8, 9,10,11,12,13 },
    { 4, 5, 6, 7, 8, 9,10,11,12 },
    { 3, 4, 5, 6, 7, 8, 9,10,11 },
    { 2, 3, 4, 5, 6, 7, 8, 9,10 },
    { 1, 2, 3, 4, 5, 6, 7, 8, 9 },
};

/* offsets from "p" */
static vx_int32 offsets[16][2] = 
{
    { 0, -3 },
    { 1, -3 },
    { 2, -2 },
    { 3, -1 },
    { 3,  0 },
    { 3,  1 },
    { 2,  2 },
    { 1,  3 },
    { 0,  3 },
    { -1,  3 },
    { -2,  2 },
    { -3,  1 },
    { -3,  0 },
    { -3, -1 },
    { -2, -2 },
    { -1, -3 },
};


static vx_bool vxIsFastCorner(const vx_uint8* buf, vx_uint8 p, vx_uint8 tolerance)
{
    vx_int32 i, a;
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
                                        vx_imagepatch_addressing_t* src_addr, vx_uint8 tolerance)
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
            buf[j] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + offsets[j][0], y + offsets[j][1], src_addr);
        }

        if (!vxIsFastCorner(buf, p, tolerance))
            return 0;

        a = tolerance;
        while (b - a > 1)
        {
            vx_uint8 c = (a + b) / 2;
            if (vxIsFastCorner(buf, p, c))
                a = c;
            else
                b = c;
        }
        return a;
    }
}

void Fast9Corners_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_float32 *sens = (vx_float32*)parameters[1];
    vx_bool *nonm = (vx_bool*)parameters[2];
    vx_tile_array_t *points = (vx_tile_array_t *)parameters[3];
    vx_uint32 *s_num_corners = (vx_uint32 *)parameters[4];

    vx_keypoint_t kp;

    vx_size num_corners = 0;

    vx_uint8 *src_base = in->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = in->tile_y + in->tile_block.height;

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = in->tile_x + in->tile_block.width;

    vx_uint8 tolerance = (vx_uint8)(*sens);
    vx_bool do_nonmax = *nonm;
    vx_size dst_capacity = points->capacity;

    memset(&kp, 0, sizeof(kp));

    vx_int32 w8 = ((in->image.width - 2 * APERTURE) >> 3) << 3;
    vx_int16 szXStep[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    vx_uint8 szStrength[8];
    int16x8_t vXStep = vld1q_s16(szXStep);
    uint8x8_t vZero = vdup_n_u8(0);
    uint8x8_t vPrv[7], vCur[7], vNxt[7];
    uint8x8_t vNMPrv[7], vNMCur[7], vNMNxt[7];

    if (high_y == in->image.height && high_x == in->image.width)
    {
        for (y = APERTURE; y < in->image.height - APERTURE; y++)
        {
            for (vx_uint8 idx = 0; idx < 7; idx++)
            {
                vPrv[idx] = vdup_n_u8(0);
                vCur[idx] = vld1_u8((vx_uint8 *)src_base + (y - APERTURE + idx) * in->addr->stride_y);
            }
            for (x = 0; x < in->image.width - APERTURE; x += 8)
            {
                for (vx_uint8 idx = 0; idx < 7; idx++)
                {
                    vNxt[idx] = vld1_u8((vx_uint8 *)src_base + (y - APERTURE + idx) * in->addr->stride_y + (x + 8) * in->addr->stride_x);
                }
                int16x8_t vX = vaddq_s16(vdupq_n_s16(x), vXStep);

                memset(szStrength, 0, 8);
                fast9CornersPerRow(vPrv, vCur, vNxt, &vX, in->addr, tolerance, szStrength);
                uint8x8_t vStrength = vld1_u8(szStrength);
                uint8x8_t vPred = vcgt_u8(vStrength, vZero);
                uint64x1_t vRetBit = vreinterpret_u64_u8(vPred);

                if (do_nonmax && (0 != vget_lane_u64(vRetBit, 0)))
                {
                    vx_uint8 szNMStrength[8];
                    uint8x8_t vTmpPrv[7], vTmpCur[7], vTmpNxt[7];
                    uint8x8_t vNMStrength;
                    uint8x8_t vTmpPred;
                    int16x8_t vNMX;
                    if ((y - 1) >= APERTURE)
                    {
                        if (x != 0)
                        {
                            vNMPrv[0] = vld1_u8((vx_uint8 *)src_base + (y - APERTURE - 1) * in->addr->stride_y + (x - 8) * in->addr->stride_x);
                        }
                        else
                        {
                            vNMPrv[0] = vdup_n_u8(0);
                        }
                        vNMCur[0] = vld1_u8((vx_uint8 *)src_base + (y - APERTURE - 1) * in->addr->stride_y + x * in->addr->stride_x);
                        vNMNxt[0] = vld1_u8((vx_uint8 *)src_base + (y - APERTURE - 1) * in->addr->stride_y + (x + 8) * in->addr->stride_x);
                        for (vx_uint8 idx = 1; idx < 7; idx++)
                        {
                            vNMPrv[idx] = vPrv[idx - 1];
                            vNMCur[idx] = vCur[idx - 1];
                            vNMNxt[idx] = vNxt[idx - 1];
                        }

                        for (vx_uint8 idx = 0; idx < 7; idx++)
                        {
                            vTmpPrv[idx] = vext_u8(vZero, vNMPrv[idx], 7);
                            vTmpCur[idx] = vext_u8(vNMPrv[idx], vNMCur[idx], 7);
                            vTmpNxt[idx] = vext_u8(vNMCur[idx], vNMNxt[idx], 7);
                        }
                        vNMX = vsubq_s16(vX, vdupq_n_s16(1));
                        memset(szNMStrength, 0, 8);
                        fast9CornersPerRow(vTmpPrv, vTmpCur, vTmpNxt, &vNMX, in->addr, tolerance, szNMStrength);
                        vNMStrength = vld1_u8(szNMStrength);
                        vTmpPred = vcge_u8(vStrength, vNMStrength);
                        vPred = vand_u8(vPred, vTmpPred);
                        vRetBit = vreinterpret_u64_u8(vPred);

                        if (0 != vget_lane_u64(vRetBit, 0))
                        {
                            memset(szNMStrength, 0, 8);
                            fast9CornersPerRow(vNMPrv, vNMCur, vNMNxt, &vX, in->addr, tolerance, szNMStrength);
                            vNMStrength = vld1_u8(szNMStrength);
                            vTmpPred = vcge_u8(vStrength, vNMStrength);
                            vPred = vand_u8(vPred, vTmpPred);
                            vRetBit = vreinterpret_u64_u8(vPred);
                        }

                        if (0 != vget_lane_u64(vRetBit, 0))
                        {
                            for (vx_uint8 idx = 0; idx < 7; idx++)
                            {
                                vTmpPrv[idx] = vext_u8(vNMPrv[idx], vNMCur[idx], 1);
                                vTmpCur[idx] = vext_u8(vNMCur[idx], vNMNxt[idx], 1);
                                vTmpNxt[idx] = vext_u8(vNMNxt[idx], vZero, 1);
                            }
                            vNMX = vaddq_s16(vX, vdupq_n_s16(1));
                            memset(szNMStrength, 0, 8);
                            fast9CornersPerRow(vTmpPrv, vTmpCur, vTmpNxt, &vNMX, in->addr, tolerance, szNMStrength);
                            vNMStrength = vld1_u8(szNMStrength);
                            vTmpPred = vcge_u8(vStrength, vNMStrength);
                            vPred = vand_u8(vPred, vTmpPred);
                            vRetBit = vreinterpret_u64_u8(vPred);
                        }
                    }

                    if (0 != vget_lane_u64(vRetBit, 0))
                    {
                        for (vx_uint8 idx = 0; idx < 7; idx++)
                        {
                            vTmpPrv[idx] = vext_u8(vZero, vPrv[idx], 7);
                            vTmpCur[idx] = vext_u8(vPrv[idx], vCur[idx], 7);
                            vTmpNxt[idx] = vext_u8(vCur[idx], vNxt[idx], 7);
                        }
                        vNMX = vsubq_s16(vX, vdupq_n_s16(1));
                        memset(szNMStrength, 0, 8);
                        fast9CornersPerRow(vTmpPrv, vTmpCur, vTmpNxt, &vNMX, in->addr, tolerance, szNMStrength);
                        vNMStrength = vld1_u8(szNMStrength);
                        vTmpPred = vcge_u8(vStrength, vNMStrength);
                        vPred = vand_u8(vPred, vTmpPred);
                        vRetBit = vreinterpret_u64_u8(vPred);
                    }

                    if (0 != vget_lane_u64(vRetBit, 0))
                    {
                        for (vx_uint8 idx = 0; idx < 7; idx++)
                        {
                            vTmpPrv[idx] = vext_u8(vPrv[idx], vCur[idx], 1);
                            vTmpCur[idx] = vext_u8(vCur[idx], vNxt[idx], 1);
                            vTmpNxt[idx] = vext_u8(vNxt[idx], vZero, 1);
                        }
                        vNMX = vaddq_s16(vX, vdupq_n_s16(1));
                        memset(szNMStrength, 0, 8);
                        fast9CornersPerRow(vTmpPrv, vTmpCur, vTmpNxt, &vNMX, in->addr, tolerance, szNMStrength);
                        vNMStrength = vld1_u8(szNMStrength);
                        vTmpPred = vcgt_u8(vStrength, vNMStrength);
                        vPred = vand_u8(vPred, vTmpPred);
                        vRetBit = vreinterpret_u64_u8(vPred);
                    }

                    if ((y + 1) < (in->image.height - APERTURE))
                    {
                        if (0 != vget_lane_u64(vRetBit, 0))
                        {
                            if (x != 0)
                            {
                                vNMPrv[6] = vld1_u8((vx_uint8 *)src_base + (y + APERTURE + 1) * in->addr->stride_y + (x - 8) * in->addr->stride_x);
                            }
                            else
                            {
                                vNMPrv[6] = vdup_n_u8(0);
                            }
                            vNMCur[6] = vld1_u8((vx_uint8 *)src_base + (y + APERTURE + 1) * in->addr->stride_y + x * in->addr->stride_x);
                            vNMNxt[6] = vld1_u8((vx_uint8 *)src_base + (y + APERTURE + 1) * in->addr->stride_y + (x + 8) * in->addr->stride_x);
                            for (vx_uint8 idx = 0; idx < 6; idx++)
                            {
                                vNMPrv[idx] = vPrv[idx + 1];
                                vNMCur[idx] = vCur[idx + 1];
                                vNMNxt[idx] = vNxt[idx + 1];
                            }

                            for (vx_uint8 idx = 0; idx < 7; idx++)
                            {
                                vTmpPrv[idx] = vext_u8(vZero, vNMPrv[idx], 7);
                                vTmpCur[idx] = vext_u8(vNMPrv[idx], vNMCur[idx], 7);
                                vTmpNxt[idx] = vext_u8(vNMCur[idx], vNMNxt[idx], 7);
                            }
                            vNMX = vsubq_s16(vX, vdupq_n_s16(1));
                            memset(szNMStrength, 0, 8);
                            fast9CornersPerRow(vTmpPrv, vTmpCur, vTmpNxt, &vNMX, in->addr, tolerance, szNMStrength);
                            vNMStrength = vld1_u8(szNMStrength);
                            vTmpPred = vcgt_u8(vStrength, vNMStrength);
                            vPred = vand_u8(vPred, vTmpPred);
                            vRetBit = vreinterpret_u64_u8(vPred);
                        }

                        if (0 != vget_lane_u64(vRetBit, 0))
                        {
                            memset(szNMStrength, 0, 8);
                            fast9CornersPerRow(vNMPrv, vNMCur, vNMNxt, &vX, in->addr, tolerance, szNMStrength);
                            vNMStrength = vld1_u8(szNMStrength);
                            vTmpPred = vcgt_u8(vStrength, vNMStrength);
                            vPred = vand_u8(vPred, vTmpPred);
                            vRetBit = vreinterpret_u64_u8(vPred);
                        }

                        if (0 != vget_lane_u64(vRetBit, 0))
                        {
                            for (vx_uint8 idx = 0; idx < 7; idx++)
                            {
                                vTmpPrv[idx] = vext_u8(vNMPrv[idx], vNMCur[idx], 1);
                                vTmpCur[idx] = vext_u8(vNMCur[idx], vNMNxt[idx], 1);
                                vTmpNxt[idx] = vext_u8(vNMNxt[idx], vZero, 1);
                            }
                            vNMX = vaddq_s16(vX, vdupq_n_s16(1));
                            memset(szNMStrength, 0, 8);
                            fast9CornersPerRow(vTmpPrv, vTmpCur, vTmpNxt, &vNMX, in->addr, tolerance, szNMStrength);
                            vNMStrength = vld1_u8(szNMStrength);
                            vTmpPred = vcgt_u8(vStrength, vNMStrength);
                            vPred = vand_u8(vPred, vTmpPred);
                        }
                    }
                }

                vRetBit = vreinterpret_u64_u8(vPred);
                if (0 != vget_lane_u64(vRetBit, 0))
                {
                    addCorner(y, &vX, &vPred, szStrength, dst_capacity, &num_corners, points);
                }

                for (vx_uint8 idx = 0; idx < 7; idx++)
                {
                    vPrv[idx] = vCur[idx];
                    vCur[idx] = vNxt[idx];
                }
            }
        }
        if (s_num_corners)
            *s_num_corners = num_corners;
    }
}


#define FAST9CORNERS(low_y, high_y, low_x, high_x)                                                              \
    for (y = low_y; y < high_y; y++)                                                                            \
    {                                                                                                           \
        for (x = low_x; x < high_x; x++)                                                                        \
        {                                                                                                       \
            vx_uint8 strength = vxGetFastCornerStrength(x, y, src_base, in->addr, tolerance);                   \
            if (strength > 0)                                                                                   \
            {                                                                                                   \
                if (do_nonmax)                                                                                  \
                {                                                                                               \
                    if (strength >= vxGetFastCornerStrength(x - 1, y - 1, src_base, in->addr, tolerance) &&     \
                        strength >= vxGetFastCornerStrength(x, y - 1, src_base, in->addr, tolerance) &&         \
                        strength >= vxGetFastCornerStrength(x + 1, y - 1, src_base, in->addr, tolerance) &&     \
                        strength >= vxGetFastCornerStrength(x - 1, y, src_base, in->addr, tolerance) &&         \
                        strength > vxGetFastCornerStrength(x + 1, y, src_base, in->addr, tolerance) &&          \
                        strength > vxGetFastCornerStrength(x - 1, y + 1, src_base, in->addr, tolerance) &&      \
                        strength > vxGetFastCornerStrength(x, y + 1, src_base, in->addr, tolerance) &&          \
                        strength > vxGetFastCornerStrength(x + 1, y + 1, src_base, in->addr, tolerance))        \
                        ;                                                                                       \
                    else                                                                                        \
                        continue;                                                                               \
                }                                                                                               \
                if (num_corners < dst_capacity)                                                                 \
                {                                                                                               \
                    kp.x = x;                                                                                   \
                    kp.y = y;                                                                                   \
                    kp.strength = strength;                                                                     \
                    kp.scale = 0.0f;                                                                            \
                    kp.orientation = 0.0f;                                                                      \
                    kp.tracking_status = 1;                                                                     \
                    kp.error = 0.0f;                                                                            \
                    vxAddArrayItems_tiling(points, 1, &kp, sizeof(kp));                                         \
                }                                                                                               \
                num_corners++;                                                                                  \
            }                                                                                                   \
        }                                                                                                       \
    }


void Fast9Corners_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_float32 *sens = (vx_float32*)parameters[1];
    vx_bool *nonm = (vx_bool*)parameters[2];
    vx_tile_array_t *points = (vx_tile_array_t *)parameters[3];
    vx_uint32 *s_num_corners = (vx_uint32 *)parameters[4];

    vx_keypoint_t kp;

    vx_size num_corners = 0;

    vx_uint8 *src_base = in->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_uint8 tolerance = (vx_uint8)(*sens);
    vx_bool do_nonmax = *nonm;
    vx_size dst_capacity = points->capacity;

    memset(&kp, 0, sizeof(kp));

    if (low_y == 0 && low_x == 0)
    {
        FAST9CORNERS(low_y + APERTURE, high_y - APERTURE, low_x + APERTURE, high_x - APERTURE)
    }
    else
    {
        FAST9CORNERS(APERTURE, low_y, low_x, high_x - APERTURE)
        FAST9CORNERS(low_y, high_y, APERTURE, high_x - APERTURE)
    }

    if (s_num_corners)
        *s_num_corners = num_corners;
}
