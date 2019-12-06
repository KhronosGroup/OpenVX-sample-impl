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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define VX_PI   3.1415926535897932384626433832795
static vx_uint64 state = 0xffffffff; 

vx_int32 vxRound(vx_float64 value)
{
    return (vx_int32)(value + (value >= 0 ? 0.5 : -0.5));
}

int32x4_t vxRound_s32x4(float32x4_t value, int32x4_t acc)
{
    if(vgetq_lane_f32(value, 0) >= 0)
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 0) + 0.5 + vgetq_lane_s32(acc, 0), value, 0);
    }
    else
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 0) - 0.5 + vgetq_lane_s32(acc, 0), value, 0);
    }
    if(vgetq_lane_f32(value, 1) >= 0)
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 1) + 0.5 + vgetq_lane_s32(acc, 1), value, 1);
    }
    else
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 1) - 0.5 + vgetq_lane_s32(acc, 1), value, 1);
    }
    if(vgetq_lane_f32(value, 2) >= 0)
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 2) + 0.5 + vgetq_lane_s32(acc, 2), value, 2);
    }
    else
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 2) - 0.5 + vgetq_lane_s32(acc, 2), value, 2);
    }
    if(vgetq_lane_f32(value, 3) >= 0)
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 3) + 0.5 + vgetq_lane_s32(acc, 3), value, 3);
    }
    else
    {
        value = vsetq_lane_f32(vgetq_lane_f32(value, 3) - 0.5 + vgetq_lane_s32(acc, 3), value, 3);
    }
    return vcvtq_s32_f32(value);
}

void vx_rng(vx_uint64 _state) { state = _state ? _state : 0xffffffff; }

unsigned vx_rng_next()
{
    state = (vx_uint64)(unsigned)state * /*CV_RNG_COEFF*/ 4164903690U + (unsigned)(state >> 32);
    return (unsigned)state;
}

vx_int32 vx_rng_uniform(vx_int32 a, vx_int32 b) { return a == b ? a : (vx_int32)(vx_rng_next() % (b - a) + a); }

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

static void vxMapArrayRange_tiling(vx_tile_array_t *arr, vx_uint8 *buf, vx_size start, vx_size end,  vx_size *stride, void **ptr)
{
    vx_size size = (end - start) * arr->item_size;
    vx_uint8 *pSrc = (vx_uint8 *)arr->ptr;
    vx_uint8 *pDst = (vx_uint8 *)buf;
    memcpy(pDst, pSrc, size);
    *ptr = buf;
}

void HoughLinesP_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_array_t *param_hough_lines_array = (vx_tile_array_t *)parameters[1];
    vx_tile_array_t *lines_array = (vx_tile_array_t *)parameters[2];
    vx_size *num_lines = (vx_size *)parameters[3];
    
    if (in->image.format != VX_DF_IMAGE_U8)
    {
        return;
    }
    
    vx_uint32 low_height = in->tile_y;
    vx_uint32 height_tiling = in->tile_y + in->tile_block.height;    
    vx_uint32 low_width = in->tile_x;
    vx_uint32 width_tiling = in->tile_x + in->tile_block.width;    
    if ((in->image.height - height_tiling) <  in->tile_block.height  && (in->image.width - width_tiling) < in->tile_block.width)
    {
        vx_coordinates2d_t pt;
        vx_uint8* buf = malloc(param_hough_lines_array->item_size*sizeof(vx_uint8));
        void *param_hough_lines_array_ptr;
        vxMapArrayRange_tiling(param_hough_lines_array, buf, 0, param_hough_lines_array->num_items, 0, &param_hough_lines_array_ptr);
        vx_hough_lines_p_t *param_hough_lines = (vx_hough_lines_p_t *)param_hough_lines_array_ptr;
        vx_float32 irho = 1 / param_hough_lines->rho;
        vx_int32 width, height;

        void *src_base = in->base[0];
        width = in->addr[0].dim_x;
        height = in->addr[0].dim_y;

        int numangle = VX_PI/param_hough_lines->theta;
        int numrho = vxRound(((width + height) * 2 + 1) / param_hough_lines->rho);
        vx_rng((vx_uint64)-1);
        vx_context context_houghlines_internal = vxCreateContext();
        vx_array accum = vxCreateArray(context_houghlines_internal, VX_TYPE_INT32, numrho * numangle);
        vx_array mask = vxCreateArray(context_houghlines_internal, VX_TYPE_UINT8, width * height);
        vx_array trigtab = vxCreateArray(context_houghlines_internal, VX_TYPE_FLOAT32, numangle * 2);
        vx_array nzloc = vxCreateArray(context_houghlines_internal, VX_TYPE_COORDINATES2D, width * height);
     
        vx_size accum_stride = 0;
        void *accum_ptr = NULL;
        vx_map_id accum_map_id;
        for (int i = 0; i < numrho * numangle; i++)
        {
            vx_int32 num = 0;
            vxAddArrayItems(accum, 1, &num, VX_TYPE_INT32);
        }
        vxMapArrayRange(accum, 0, numrho * numangle, &accum_map_id, &accum_stride, &accum_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        vx_int32 *accum_p = (vx_int32 *)accum_ptr;

        for (int n = 0; n < numangle; n++)
        {
            vx_float32 cos_v = (vx_float32)(cos((vx_float64)n * (param_hough_lines->theta)) * irho);
            vx_float32 sin_v = (vx_float32)(sin((vx_float64)n * (param_hough_lines->theta)) * irho);
            vxAddArrayItems(trigtab, 1, &cos_v, sizeof(vx_float32));
            vxAddArrayItems(trigtab, 1, &sin_v, sizeof(vx_float32));
        }

        vx_size trigtab_stride = 0;
        void *trigtab_ptr = NULL;
        vx_map_id trigtab_map_id;
        vxMapArrayRange(trigtab, 0, numangle * 2, &trigtab_map_id, &trigtab_stride, &trigtab_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        vx_float32 *trigtab_p = (vx_float32 *)trigtab_ptr;
        const vx_float32* ttab = &trigtab_p[0];

        vx_int32 nzcount = 0;
        vx_size lines_num = 0;

        // stage 1. collect non-zero image points  
        for (pt.y = 0; pt.y < height; pt.y++)
        {
            const vx_uint8 *data = (vx_uint8 *)in->base[0] + pt.y*in->addr[0].stride_y;
            for (pt.x = 0; pt.x < width; pt.x++)
            {
                vx_uint8 mdata_in = 0;
                if (data[pt.x])
                {
                    mdata_in = 1;
                    vxAddArrayItems(nzloc, 1, &pt, sizeof(vx_coordinates2d_t));
                    nzcount++;
                }
                vxAddArrayItems(mask, 1, &mdata_in, sizeof(vx_uint8));
            }
        }

        vx_size nzloc_stride = 0;
        void *nzloc_ptr = NULL;
        vx_map_id nzloc_map_id;

        vx_size mask_stride = 0;
        void *mask_ptr = NULL;
        vx_map_id mask_map_id;

        vxMapArrayRange(mask, 0, width * height, &mask_map_id, &mask_stride, &mask_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        vxMapArrayRange(nzloc, 0, nzcount, &nzloc_map_id, &nzloc_stride, &nzloc_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        vx_uint8 *mask_p = (vx_uint8 *)mask_ptr;
        vx_coordinates2d_t *nzloc_p = (vx_coordinates2d_t *)nzloc_ptr;

        vx_uint8* mdata0 = mask_p;
        // stage 2. process all the points in random order
        vx_int32 roiw3 = numangle >= 3 ? numangle - 3 : 0;
        float32x4_t r_f32x4;
        int32x4_t r_s32x4;
        int32x4_t val_s32x4;
        int32x4_t n_s32x4;
        vx_int32 numrho_tmp =  (numrho - 1) / 2;
        int32x4_t numrho_s32x4 = vdupq_n_s32(numrho_tmp);
        for (; nzcount > 0; nzcount--)
        {
            // choose random point out of the remaining ones
            vx_int32 idx = vx_rng_uniform(0, nzcount);
            vx_int32 max_val = param_hough_lines->threshold - 1, max_n = 0;
            vx_coordinates2d_t point = nzloc_p[idx];
            vx_coordinates2d_t line_end[2];
            vx_float32 a, b;
            vx_int32* adata = accum_p;
            vx_int32 i = point.y, j = point.x, k, x0, y0, dx0, dy0, xflag;
            vx_int32 good_line;
            const vx_int32 shift = 16;

            // "remove" it by overriding it with the last element
            nzloc_p[idx] = nzloc_p[nzcount - 1];

            // check if it has been excluded already (i.e. belongs to some other line)
            if (mdata0 != NULL && !mdata0[i*width + j])
                continue;

            // update accumulator, find the most probable line
            vx_int32 n = 0;
#if 0
            for (; n < roiw3; n+=4)
            {
                r_f32x4 = vsetq_lane_f32(j * ttab[n * 2] + i * ttab[n * 2 + 1], r_f32x4, 0);
                r_f32x4 = vsetq_lane_f32(j * ttab[(n+1) * 2] + i * ttab[(n+1) * 2 + 1], r_f32x4, 1);
                r_f32x4 = vsetq_lane_f32(j * ttab[(n+2) * 2] + i * ttab[(n+2) * 2 + 1], r_f32x4, 2);
                r_f32x4 = vsetq_lane_f32(j * ttab[(n+3) * 2] + i * ttab[(n+3) * 2 + 1], r_f32x4, 3);
                r_s32x4 = vxRound_s32x4(r_f32x4, numrho_s32x4);
                val_s32x4 = vsetq_lane_s32(++adata[vgetq_lane_s32(r_s32x4, 0)], val_s32x4, 0);
                val_s32x4 = vsetq_lane_s32(++adata[vgetq_lane_s32(r_s32x4, 1)], val_s32x4, 1);
                val_s32x4 = vsetq_lane_s32(++adata[vgetq_lane_s32(r_s32x4, 2)], val_s32x4, 2);
                val_s32x4 = vsetq_lane_s32(++adata[vgetq_lane_s32(r_s32x4, 3)], val_s32x4, 3);
                if (max_val < vgetq_lane_s32(val_s32x4, 0))
                {
                    max_val = vgetq_lane_s32(val_s32x4, 0);
                    max_n = n;
                }
                if (max_val < vgetq_lane_s32(val_s32x4, 1))
                {
                    max_val = vgetq_lane_s32(val_s32x4, 1);
                    max_n = n+1;
                }
                if (max_val < vgetq_lane_s32(val_s32x4, 2))
                {
                    max_val = vgetq_lane_s32(val_s32x4, 2);
                    max_n = n+2;
                }
                if (max_val < vgetq_lane_s32(val_s32x4, 3))
                {
                    max_val = vgetq_lane_s32(val_s32x4, 3);
                    max_n = n+3;
                }
                adata += 4*numrho;
            }
#endif
            for (; n < numangle; n++)
            {
                vx_int32 r = vxRound(j * ttab[n * 2] + i * ttab[n * 2 + 1]);
                r += numrho_tmp;
                vx_int32 val = ++adata[r];
                if (max_val < val)
                {
                    max_val = val;
                    max_n = n;
                }
                adata += numrho;
            }

            // if it is too "weak" candidate, continue with another point
            if (max_val < param_hough_lines->threshold)
                continue;

            // from the current point walk in each direction
            // along the found line and extract the line segment
            a = -ttab[max_n * 2 + 1];
            b = ttab[max_n * 2];
            x0 = j;
            y0 = i;
            if (fabs(a) > fabs(b))
            {
                xflag = 1;
                dx0 = a > 0 ? 1 : -1;
                dy0 = vxRound(b*(1 << shift) / fabs(a));
                y0 = (y0 << shift) + (1 << (shift - 1));
            }
            else
            {
                xflag = 0;
                dy0 = b > 0 ? 1 : -1;
                dx0 = vxRound(a*(1 << shift) / fabs(b));
                x0 = (x0 << shift) + (1 << (shift - 1));
            }

            for (k = 0; k < 2; k++)
            {
                vx_int32 gap = 0, x = x0, y = y0, dx = dx0, dy = dy0;

                if (k > 0)
                    dx = -dx, dy = -dy;

                // walk along the line using fixed-point arithmetics,
                // stop at the image border or in case of too big gap
                for (;; x += dx, y += dy)
                {
                    vx_uint8* mdata;
                    vx_int32 i1, j1;

                    if (xflag)
                    {
                        j1 = x;
                        i1 = y >> shift;
                    }
                    else
                    {
                        j1 = x >> shift;
                        i1 = y;
                    }

                    if (j1 < 0 || j1 >= width || i1 < 0 || i1 >= height)
                        break;

                    mdata = mdata0 + i1*width + j1;

                    // for each non-zero point:
                    //    update line end,
                    //    clear the mask element
                    //    reset the gap
                    if (*mdata)
                    {
                        gap = 0;
                        line_end[k].y = i1;
                        line_end[k].x = j1;
                    }
                    else if (++gap > param_hough_lines->line_gap)
                        break;
                }
            }

            good_line = abs((int)line_end[1].x - (int)line_end[0].x) >= param_hough_lines->line_length ||
                        abs((int)line_end[1].y - (int)line_end[0].y) >= param_hough_lines->line_length;

            for (k = 0; k < 2; k++)
            {
                vx_int32 x = x0, y = y0, dx = dx0, dy = dy0;

                if (k > 0)
                    dx = -dx, dy = -dy;

                // walk along the line using fixed-point arithmetics,
                // stop at the image border or in case of too big gap
                for (;; x += dx, y += dy)
                {
                    vx_uint8* mdata;
                    vx_int32 i1, j1;

                    if (xflag)
                    {
                        j1 = x;
                        i1 = y >> shift;
                    }
                    else
                    {
                        j1 = x >> shift;
                        i1 = y;
                    }

                    mdata = mdata0 + i1*width + j1;

                    // for each non-zero point:
                    //    update line end,
                    //    clear the mask element
                    //    reset the gap
                    if (*mdata)
                    {
                        if (good_line)
                        {
                            adata = accum_p;
                            for (vx_int32 n = 0; n < numangle; n++, adata += numrho)
                            {
                                vx_int32 r = vxRound(j1 * ttab[n * 2] + i1 * ttab[n * 2 + 1]);
                                r += (numrho - 1) / 2;
                                adata[r]--;
                            }
                        }
                        *mdata = 0;
                    }

                    if (i1 == line_end[k].y && j1 == line_end[k].x)
                        break;
                }
            }

            if (good_line)
            {
                vx_line2d_t lines;
                if ((line_end[0].x > line_end[1].x) || (line_end[0].x == line_end[1].x && line_end[0].y > line_end[1].y))
                {
                    lines.start_x = line_end[1].x;
                    lines.start_y = line_end[1].y;
                    lines.end_x = line_end[0].x;
                    lines.end_y = line_end[0].y;
                }
                else
                {
                    lines.start_x = line_end[0].x;
                    lines.start_y = line_end[0].y;
                    lines.end_x = line_end[1].x;
                    lines.end_y = line_end[1].y;
                }
                //vxAddArrayItems(lines_array, 1, &lines, sizeof(vx_line2d_t));
                vxAddArrayItems_tiling(lines_array, 1, &lines, sizeof(vx_line2d_t));
                lines_num++;
            }
        }
        vxUnmapArrayRange(accum, accum_map_id);
        vxUnmapArrayRange(mask, mask_map_id);
        vxUnmapArrayRange(trigtab, trigtab_map_id);
        vxUnmapArrayRange(nzloc, nzloc_map_id);
        vxReleaseArray(&accum);
        vxReleaseArray(&mask);
        vxReleaseArray(&trigtab);
        vxReleaseArray(&nzloc);
        free(buf);
    }
}

void HoughLinesP_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_array_t *param_hough_lines_array = (vx_tile_array_t *)parameters[1];
    vx_tile_array_t *lines_array = (vx_tile_array_t *)parameters[2];
    vx_size *num_lines = (vx_size *)parameters[3];
    if (in->image.format != VX_DF_IMAGE_U1 && in->image.format != VX_DF_IMAGE_U8)
    {
        return;
    }
    
    vx_coordinates2d_t pt;
    vx_uint8* buf = malloc(param_hough_lines_array->item_size*sizeof(vx_uint8));
    void *param_hough_lines_array_ptr;
    vxMapArrayRange_tiling(param_hough_lines_array, buf, 0, param_hough_lines_array->num_items, 0, &param_hough_lines_array_ptr);
    vx_hough_lines_p_t *param_hough_lines = (vx_hough_lines_p_t *)param_hough_lines_array_ptr;
    vx_float32 irho = 1 / param_hough_lines->rho;
    vx_int32 width, height;

    void *src_base = in->base[0];
    width = vxTileWidth(in, 0);//in->addr[0].dim_x;
    height = vxTileHeight(in, 0);//in->addr[0].dim_y;

    int numangle = VX_PI/param_hough_lines->theta;
    int numrho = vxRound(((width + height) * 2 + 1) / param_hough_lines->rho);
    vx_rng((vx_uint64)-1);
    vx_context context_houghlines_internal = vxCreateContext();
    vx_array accum = vxCreateArray(context_houghlines_internal, VX_TYPE_INT32, numrho * numangle);
    vx_array mask = vxCreateArray(context_houghlines_internal, VX_TYPE_UINT8, width * height);
    vx_array trigtab = vxCreateArray(context_houghlines_internal, VX_TYPE_FLOAT32, numangle * 2);
    vx_array nzloc = vxCreateArray(context_houghlines_internal, VX_TYPE_COORDINATES2D, width * height);
 
    vx_size accum_stride = 0;
    void *accum_ptr = NULL;
    vx_map_id accum_map_id;
    for (int i = 0; i < numrho * numangle; i++)
    {
        vx_int32 num = 0;
        vxAddArrayItems(accum, 1, &num, VX_TYPE_INT32);
    }
    vxMapArrayRange(accum, 0, numrho * numangle, &accum_map_id, &accum_stride, &accum_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    vx_int32 *accum_p = (vx_int32 *)accum_ptr;

    for (int n = 0; n < numangle; n++)
    {
        vx_float32 cos_v = (vx_float32)(cos((vx_float64)n * (param_hough_lines->theta)) * irho);
        vx_float32 sin_v = (vx_float32)(sin((vx_float64)n * (param_hough_lines->theta)) * irho);
        vxAddArrayItems(trigtab, 1, &cos_v, sizeof(vx_float32));
        vxAddArrayItems(trigtab, 1, &sin_v, sizeof(vx_float32));
    }

    vx_size trigtab_stride = 0;
    void *trigtab_ptr = NULL;
    vx_map_id trigtab_map_id;
    vxMapArrayRange(trigtab, 0, numangle * 2, &trigtab_map_id, &trigtab_stride, &trigtab_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    vx_float32 *trigtab_p = (vx_float32 *)trigtab_ptr;
    const vx_float32* ttab = &trigtab_p[0];

    vx_int32 nzcount = 0;
    vx_size lines_num = 0;

    // stage 1. collect non-zero image points
    vx_uint32 ty = in->tile_y;
    vx_uint32 tx = in->tile_x;
    if (ty == 0 && tx == 0)
    {
        for (pt.y = 0; pt.y < vxTileHeight(in, 0); pt.y++) 
        {
            const vx_uint8 *data = (vx_uint8 *)in->base[0] + pt.y*in->addr[0].stride_y;
            for (pt.x = 0; pt.x < vxTileWidth(in, 0); pt.x++) 
            {
                vx_uint8 mdata_in = 0;
                vx_bool pxl_is_valid = vx_false_e, pxl_is_non_zero = vx_false_e;

                if (pt.y >= in->rect.start_y && pt.y < in->rect.end_y && pt.x >= in->rect.start_x && pt.x < in->rect.end_x)
                    pxl_is_valid = vx_true_e;

                if (pxl_is_valid && in->image.format == VX_DF_IMAGE_U1)
                    pxl_is_non_zero = (data[pt.x / 8] & (1 << (pt.x % 8))) ? vx_true_e : vx_false_e;
                else if (pxl_is_valid)  // format == VX_DF_IMAGE_U8
                    pxl_is_non_zero =  data[pt.x] ? vx_true_e : vx_false_e;

                if (pxl_is_non_zero)
                {
                    mdata_in = 1;
                    vxAddArrayItems(nzloc, 1, &pt, sizeof(vx_coordinates2d_t));
                    nzcount++;
                }
                vxAddArrayItems(mask, 1, &mdata_in, sizeof(vx_uint8));
            }
        }        
    }
    else
    {
        for (pt.y = 0; pt.y < ty; pt.y++)
        {
            const vx_uint8 *data = (vx_uint8 *)in->base[0] + pt.y*in->addr[0].stride_y;
            for (pt.x = tx; pt.x < vxTileWidth(in, 0); pt.x++) 
            {
                vx_uint8 mdata_in = 0;
                if (data[pt.x])
                {
                    mdata_in = 1;
                    vxAddArrayItems(nzloc, 1, &pt, sizeof(vx_coordinates2d_t));
                    nzcount++;
                }
                vxAddArrayItems(mask, 1, &mdata_in, sizeof(vx_uint8));
            }
        }
        for (pt.y = ty; pt.y < vxTileHeight(in, 0); pt.y++)
        {
            const vx_uint8 *data = (vx_uint8 *)in->base[0] + pt.y*in->addr[0].stride_y;
            for (pt.x = 0; pt.x < vxTileWidth(in, 0); pt.x++) 
            {
                vx_uint8 mdata_in = 0;
                if (data[pt.x])
                {
                    mdata_in = 1;
                    vxAddArrayItems(nzloc, 1, &pt, sizeof(vx_coordinates2d_t));
                    nzcount++;
                }
                vxAddArrayItems(mask, 1, &mdata_in, sizeof(vx_uint8));
            }
        }
        
    }

    vx_size nzloc_stride = 0;
    void *nzloc_ptr = NULL;
    vx_map_id nzloc_map_id;

    vx_size mask_stride = 0;
    void *mask_ptr = NULL;
    vx_map_id mask_map_id;

    vxMapArrayRange(mask, 0, width * height, &mask_map_id, &mask_stride, &mask_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    vxMapArrayRange(nzloc, 0, nzcount, &nzloc_map_id, &nzloc_stride, &nzloc_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    vx_uint8 *mask_p = (vx_uint8 *)mask_ptr;
    vx_coordinates2d_t *nzloc_p = (vx_coordinates2d_t *)nzloc_ptr;

    vx_uint8* mdata0 = mask_p;
    // stage 2. process all the points in random order
    for (; nzcount > 0; nzcount--)
    {
        // choose random point out of the remaining ones
        vx_int32 idx = vx_rng_uniform(0, nzcount);
        vx_int32 max_val = param_hough_lines->threshold - 1, max_n = 0;
        vx_coordinates2d_t point = nzloc_p[idx];
        vx_coordinates2d_t line_end[2];
        vx_float32 a, b;
        vx_int32* adata = accum_p;
        vx_int32 i = point.y, j = point.x, k, x0, y0, dx0, dy0, xflag;
        vx_int32 good_line;
        const vx_int32 shift = 16;

        // "remove" it by overriding it with the last element
        nzloc_p[idx] = nzloc_p[nzcount - 1];

        // check if it has been excluded already (i.e. belongs to some other line)
        if (mdata0 != NULL && !mdata0[i*width + j])
            continue;

        // update accumulator, find the most probable line
        for (vx_int32 n = 0; n < numangle; n++, adata += numrho)
        {
            vx_int32 r = vxRound(j * ttab[n * 2] + i * ttab[n * 2 + 1]);
            r += (numrho - 1) / 2;
            vx_int32 val = ++adata[r];
            if (max_val < val)
            {
                max_val = val;
                max_n = n;
            }
        }

        // if it is too "weak" candidate, continue with another point
        if (max_val < param_hough_lines->threshold)
            continue;

        // from the current point walk in each direction
        // along the found line and extract the line segment
        a = -ttab[max_n * 2 + 1];
        b = ttab[max_n * 2];
        x0 = j;
        y0 = i;
        if (fabs(a) > fabs(b))
        {
            xflag = 1;
            dx0 = a > 0 ? 1 : -1;
            dy0 = vxRound(b*(1 << shift) / fabs(a));
            y0 = (y0 << shift) + (1 << (shift - 1));
        }
        else
        {
            xflag = 0;
            dy0 = b > 0 ? 1 : -1;
            dx0 = vxRound(a*(1 << shift) / fabs(b));
            x0 = (x0 << shift) + (1 << (shift - 1));
        }

        for (k = 0; k < 2; k++)
        {
            vx_int32 gap = 0, x = x0, y = y0, dx = dx0, dy = dy0;

            if (k > 0)
                dx = -dx, dy = -dy;

            // walk along the line using fixed-point arithmetics,
            // stop at the image border or in case of too big gap
            for (;; x += dx, y += dy)
            {
                vx_uint8* mdata;
                vx_int32 i1, j1;

                if (xflag)
                {
                    j1 = x;
                    i1 = y >> shift;
                }
                else
                {
                    j1 = x >> shift;
                    i1 = y;
                }

                if (j1 < 0 || j1 >= width || i1 < 0 || i1 >= height)
                    break;

                mdata = mdata0 + i1*width + j1;

                // for each non-zero point:
                //    update line end,
                //    clear the mask element
                //    reset the gap
                if (*mdata)
                {
                    gap = 0;
                    line_end[k].y = i1;
                    line_end[k].x = j1;
                }
                else if (++gap > param_hough_lines->line_gap)
                    break;
            }
        }

        good_line = abs((int)line_end[1].x - (int)line_end[0].x) >= param_hough_lines->line_length ||
                    abs((int)line_end[1].y - (int)line_end[0].y) >= param_hough_lines->line_length;

        for (k = 0; k < 2; k++)
        {
            vx_int32 x = x0, y = y0, dx = dx0, dy = dy0;

            if (k > 0)
                dx = -dx, dy = -dy;

            // walk along the line using fixed-point arithmetics,
            // stop at the image border or in case of too big gap
            for (;; x += dx, y += dy)
            {
                vx_uint8* mdata;
                vx_int32 i1, j1;

                if (xflag)
                {
                    j1 = x;
                    i1 = y >> shift;
                }
                else
                {
                    j1 = x >> shift;
                    i1 = y;
                }

                mdata = mdata0 + i1*width + j1;

                // for each non-zero point:
                //    update line end,
                //    clear the mask element
                //    reset the gap
                if (*mdata)
                {
                    if (good_line)
                    {
                        adata = accum_p;
                        for (vx_int32 n = 0; n < numangle; n++, adata += numrho)
                        {
                            vx_int32 r = vxRound(j1 * ttab[n * 2] + i1 * ttab[n * 2 + 1]);
                            r += (numrho - 1) / 2;
                            adata[r]--;
                        }
                    }
                    *mdata = 0;
                }

                if (i1 == line_end[k].y && j1 == line_end[k].x)
                    break;
            }
        }

        if (good_line)
        {
            vx_line2d_t lines;
            if ((line_end[0].x > line_end[1].x) || (line_end[0].x == line_end[1].x && line_end[0].y > line_end[1].y))
            {
                lines.start_x = line_end[1].x;
                lines.start_y = line_end[1].y;
                lines.end_x = line_end[0].x;
                lines.end_y = line_end[0].y;
            }
            else
            {
                lines.start_x = line_end[0].x;
                lines.start_y = line_end[0].y;
                lines.end_x = line_end[1].x;
                lines.end_y = line_end[1].y;
            }
            vxAddArrayItems_tiling(lines_array, 1, &lines, sizeof(vx_line2d_t));
            lines_num++;
        }
    }
    vxUnmapArrayRange(accum, accum_map_id);
    vxUnmapArrayRange(mask, mask_map_id);
    vxUnmapArrayRange(trigtab, trigtab_map_id);
    vxUnmapArrayRange(nzloc, nzloc_map_id);
    vxReleaseArray(&accum);
    vxReleaseArray(&mask);
    vxReleaseArray(&trigtab);
    vxReleaseArray(&nzloc);
    free(buf);
}
