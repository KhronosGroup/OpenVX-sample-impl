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

#include <stdlib.h>
#include <c_model.h>

#define VX_PI   3.1415926535897932384626433832795
static vx_uint64 state = 0xffffffff;

vx_int32 vxRound(vx_float64 value)
{
    return (vx_int32)(value + (value >= 0 ? 0.5 : -0.5));
}

void vx_rng(vx_uint64 _state) { state = _state ? _state : 0xffffffff; }

unsigned vx_rng_next()
{
    state = (vx_uint64)(unsigned)state * /*CV_RNG_COEFF*/ 4164903690U + (unsigned)(state >> 32);
    return (unsigned)state;
}

vx_int32 vx_rng_uniform(vx_int32 a, vx_int32 b) { return a == b ? a : (vx_int32)(vx_rng_next() % (b - a) + a); }

// nodeless version of the HoughLines Probabilistic kernel
vx_status vxHoughLinesP(vx_image img, vx_array param_hough_lines_array, vx_array lines_array, vx_scalar num_lines)
{
    vx_status status = VX_FAILURE;
    vx_coordinates2d_t pt;

    vx_size param_hough_lines_array_stride = 0;
    void *param_hough_lines_array_ptr = NULL;
    vx_map_id param_hough_lines_array_map_id;
    vx_size param_hough_lines_array_length;

    vxQueryArray(param_hough_lines_array, VX_ARRAY_NUMITEMS, &param_hough_lines_array_length, sizeof(param_hough_lines_array_length));
    vxMapArrayRange(param_hough_lines_array, 0, param_hough_lines_array_length, &param_hough_lines_array_map_id,
                    &param_hough_lines_array_stride, &param_hough_lines_array_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    vx_hough_lines_p_t *param_hough_lines = (vx_hough_lines_p_t *)param_hough_lines_array_ptr;

    vx_float32 irho = 1 / param_hough_lines->rho;

    vx_df_image format = 0;
    vx_uint32 width = 0, height = 0;
    vxQueryImage(img, VX_IMAGE_FORMAT, &format, sizeof(format));
    vxQueryImage(img, VX_IMAGE_WIDTH,  &width,  sizeof(width));
    vxQueryImage(img, VX_IMAGE_HEIGHT, &height, sizeof(height));

    if (format != VX_DF_IMAGE_U1 && format != VX_DF_IMAGE_U8)
    {
        status = VX_FAILURE;
        return status;
    }

    vx_rectangle_t src_rect, src_full_rect = {0, 0, width, height};
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    void *src_base = NULL;

    status = vxGetValidRegionImage(img, &src_rect);
    status |= vxAccessImagePatch(img, &src_full_rect, 0, &src_addr, (void **)&src_base, VX_READ_AND_WRITE);

    int numangle = (int)(((vx_float32)VX_PI) / param_hough_lines->theta);
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
    for (pt.y = 0; pt.y < (vx_uint32)height; pt.y++)
    {
        const vx_uint8* data = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, 0, pt.y, &src_addr);
        for (pt.x = 0; pt.x < (vx_uint32)width; pt.x++)
        {
            vx_uint8 mdata_in = 0;
            vx_bool pxl_is_valid = vx_false_e, pxl_is_non_zero = vx_false_e;

            if (pt.y >= src_rect.start_y && pt.y < src_rect.end_y && pt.x >= src_rect.start_x && pt.x < src_rect.end_x)
                pxl_is_valid = vx_true_e;

            if (pxl_is_valid && format == VX_DF_IMAGE_U1)
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
        if (!mdata0[i*width + j])
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

                if (j1 < 0 || j1 >= (vx_int32)width || i1 < 0 || i1 >= (vx_int32)height)
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

                if ((vx_uint32)i1 == line_end[k].y && (vx_uint32)j1 == line_end[k].x)
                    break;
            }
        }

        if (good_line)
        {
            vx_line2d_t lines;
            if ((line_end[0].x > line_end[1].x) || (line_end[0].x == line_end[1].x && line_end[0].y > line_end[1].y))
            {
                lines.start_x = (vx_float32)line_end[1].x;
                lines.start_y = (vx_float32)line_end[1].y;
                lines.end_x = (vx_float32)line_end[0].x;
                lines.end_y = (vx_float32)line_end[0].y;
            }
            else
            {
                lines.start_x = (vx_float32)line_end[0].x;
                lines.start_y = (vx_float32)line_end[0].y;
                lines.end_x = (vx_float32)line_end[1].x;
                lines.end_y = (vx_float32)line_end[1].y;
            }
            vxAddArrayItems(lines_array, 1, &lines, sizeof(vx_line2d_t));
            lines_num++;
        }
    }

    vxCopyScalar(num_lines, &lines_num, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    status |= vxUnmapArrayRange(accum, accum_map_id);
    status |= vxUnmapArrayRange(mask, mask_map_id);
    status |= vxUnmapArrayRange(trigtab, trigtab_map_id);
    status |= vxUnmapArrayRange(nzloc, nzloc_map_id);
    status |= vxUnmapArrayRange(param_hough_lines_array, param_hough_lines_array_map_id);
    status |= vxCommitImagePatch(img, &src_full_rect, 0, &src_addr, src_base);

    vxReleaseArray(&accum);
    vxReleaseArray(&mask);
    vxReleaseArray(&trigtab);
    vxReleaseArray(&nzloc);

    vxReleaseContext(&context_houghlines_internal);
    return VX_SUCCESS;
}
