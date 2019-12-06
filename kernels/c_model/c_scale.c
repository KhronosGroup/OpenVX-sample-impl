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

// helpers

/* Specification 1.0, on AREA interpolation method: "The details of this
 * sampling method are implementation defined. The implementation should
 * perform enough sampling to avoid aliasing, but there is no requirement
 * that the sample areas for adjacent output pixels be disjoint, nor that
 * the pixels be weighted evenly."
 *
 * The existing implementation of AREA can use too much heap in certain
 * circumstances, so disabling for now, and using NEAREST_NEIGHBOR instead,
 * as it also passes conformance for AREA interpolation.
 */
#define AREA_SCALE_ENABLE 0 /* TODO enable this again after changing implementation in sample/targets/vx_scale.c */

static vx_bool read_pixel_1u(void *base, vx_imagepatch_addressing_t *addr, vx_int32 x, vx_int32 y,
                             const vx_border_t *borders, vx_uint8 *pixel, vx_uint32 shift_x_u1)
{
    vx_bool out_of_bounds = (x <  (vx_int32)shift_x_u1  || y <  0 ||
                             x >= (vx_int32)addr->dim_x || y >= (vx_int32)addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 *bpixel_group;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pixel = (vx_uint8)(borders->constant_value.U1 ? 1 : 0);
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < (vx_int32)shift_x_u1 ? shift_x_u1 : x >= (vx_int32)addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0                    ? 0          : y >= (vx_int32)addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel_group = (vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = (*bpixel_group & (1 << (bx % 8))) >> (bx % 8);

    return vx_true_e;
}

static vx_bool read_pixel_8u(void *base, vx_imagepatch_addressing_t *addr,
        vx_int32 x, vx_int32 y, const vx_border_t *borders, vx_uint8 *pixel)
{
    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= (vx_int32)addr->dim_x || y >= (vx_int32)addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 *bpixel;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pixel = borders->constant_value.U8;
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < 0 ? 0 : x >= (vx_int32)addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= (vx_int32)addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = *bpixel;

    return vx_true_e;
}


static vx_bool read_pixel_16s(void *base, vx_imagepatch_addressing_t *addr,
    vx_int32 x, vx_int32 y, const vx_border_t *borders, vx_int16 *pixel)
{
    vx_uint32 bx;
    vx_uint32 by;
    vx_int16* bpixel;

    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= (vx_int32)addr->dim_x || y >= (vx_int32)addr->dim_y);

    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pixel = (vx_int16)borders->constant_value.S16;
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < 0 ? 0 : x >= (vx_int32)addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= (vx_int32)addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (vx_int16*)vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = *bpixel;

    return vx_true_e;
}


static vx_status vxNearestScaling(vx_image src_image, vx_image dst_image, const vx_border_t *borders)
{
    vx_status status = VX_SUCCESS;
    vx_int32 x1, y1, x2, y2;
    void *src_base = NULL, *dst_base = NULL;
    vx_rectangle_t src_rect, dst_rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
    vx_float32 wr, hr;
    vx_df_image format = 0;

    vxQueryImage(src_image, VX_IMAGE_WIDTH, &w1, sizeof(w1));
    vxQueryImage(src_image, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
    vxQueryImage(src_image, VX_IMAGE_FORMAT, &format, sizeof(format));

    vxQueryImage(dst_image, VX_IMAGE_WIDTH, &w2, sizeof(w2));
    vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &h2, sizeof(h2));

    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    status = VX_SUCCESS;
    status |= vxGetValidRegionImage(src_image, &src_rect);
    status |= vxAccessImagePatch(src_image, &src_rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &dst_rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    wr = (vx_float32)w1/(vx_float32)w2;
    hr = (vx_float32)h1/(vx_float32)h2;

    for (y2 = 0; y2 < (vx_int32)h2; y2++)
    {
        for (x2 = 0; x2 < (vx_int32)w2; x2++)
        {
            if (VX_DF_IMAGE_U1 == format)
            {
                vx_uint8 v = 0;
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x2, y2, &dst_addr);
                vx_float32 x_src = ((vx_float32)x2 + 0.5f)*wr - 0.5f;
                vx_float32 y_src = ((vx_float32)y2 + 0.5f)*hr - 0.5f;

                // Switch to coordinates in the mapped image patch
                x_src = x_src - src_rect.start_x + src_rect.start_x % 8;
                y_src = y_src - src_rect.start_y;
                vx_float32 x_min = floorf(x_src);
                vx_float32 y_min = floorf(y_src);
                x1 = (vx_int32)x_min;
                y1 = (vx_int32)y_min;

                if (x_src - x_min >= 0.5f)
                    x1++;
                if (y_src - y_min >= 0.5f)
                    y1++;

                if (dst && vx_true_e == read_pixel_1u(src_base, &src_addr, x1, y1, borders, &v, src_rect.start_x % 8))
                    *dst = (*dst & ~(1 << (x2 % 8))) | (v << (x2 % 8));
            }
            else if (VX_DF_IMAGE_U8 == format)
            {
                vx_uint8 v = 0;
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x2, y2, &dst_addr);
                vx_float32 x_src = ((vx_float32)x2 + 0.5f)*wr - 0.5f;
                vx_float32 y_src = ((vx_float32)y2 + 0.5f)*hr - 0.5f;

                // Switch to coordinates in the mapped image patch
                x_src = x_src - src_rect.start_x;
                y_src = y_src - src_rect.start_y;
                vx_float32 x_min = floorf(x_src);
                vx_float32 y_min = floorf(y_src);
                x1 = (vx_int32)x_min;
                y1 = (vx_int32)y_min;

                if (x_src - x_min >= 0.5f)
                    x1++;
                if (y_src - y_min >= 0.5f)
                    y1++;
                //printf("x2,y2={%u,%u} => x1,y1={%u,%u}\n", x2,y2,x1,y1);
                if (dst && vx_true_e == read_pixel_8u(src_base, &src_addr, x1, y1, borders, &v))
                    *dst = v;
            }
            else
            {
                vx_int16 v = 0;
                vx_int16* dst = (vx_int16*)vxFormatImagePatchAddress2d(dst_base, x2, y2, &dst_addr);
                vx_float32 x_src = ((vx_float32)x2 + 0.5f)*wr - 0.5f;
                vx_float32 y_src = ((vx_float32)y2 + 0.5f)*hr - 0.5f;

                // Switch to coordinates in the mapped image patch
                x_src = x_src - src_rect.start_x;
                y_src = y_src - src_rect.start_y;
                vx_float32 x_min = floorf(x_src);
                vx_float32 y_min = floorf(y_src);
                x1 = (vx_int32)x_min;
                y1 = (vx_int32)y_min;

                if (x_src - x_min >= 0.5f)
                    x1++;
                if (y_src - y_min >= 0.5f)
                    y1++;

                if (dst && vx_true_e == read_pixel_16s(src_base, &src_addr, x1, y1, borders, &v))
                    *dst = v;
            }
        }
    }

    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &dst_rect, 0, &dst_addr, dst_base);

    return VX_SUCCESS;
}

static vx_status vxBilinearScaling(vx_image src_image, vx_image dst_image, const vx_border_t *borders)
{
    vx_status status = VX_SUCCESS;
    vx_int32 x2, y2;
    void *src_base = NULL, *dst_base = NULL;
    vx_rectangle_t src_rect, dst_rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
    vx_float32 wr, hr;
    vx_df_image format = 0;

    vxQueryImage(src_image, VX_IMAGE_WIDTH, &w1, sizeof(w1));
    vxQueryImage(src_image, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
    vxQueryImage(src_image, VX_IMAGE_FORMAT, &format, sizeof(format));

    vxQueryImage(dst_image, VX_IMAGE_WIDTH, &w2, sizeof(w2));
    vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &h2, sizeof(h2));

    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    status = VX_SUCCESS;
    status |= vxGetValidRegionImage(src_image, &src_rect);
    status |= vxAccessImagePatch(src_image, &src_rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &dst_rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    wr = (vx_float32)w1/(vx_float32)w2;
    hr = (vx_float32)h1/(vx_float32)h2;

    for (y2 = 0; y2 < (vx_int32)h2; y2++)
    {
        for (x2 = 0; x2 < (vx_int32)w2; x2++)
        {
            vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;
            vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x2, y2, &dst_addr);
            vx_float32 x_src = ((vx_float32)x2+0.5f)*wr - 0.5f;
            vx_float32 y_src = ((vx_float32)y2+0.5f)*hr - 0.5f;

            // Switch to coordinates in the mapped image patch
            x_src = x_src - src_rect.start_x + (format == VX_DF_IMAGE_U1 ? src_rect.start_x % 8 : 0);
            y_src = y_src - src_rect.start_y;
            vx_float32 x_min = floorf(x_src);
            vx_float32 y_min = floorf(y_src);
            vx_int32 x1 = (vx_int32)x_min;
            vx_int32 y1 = (vx_int32)y_min;
            vx_float32 s = x_src - x_min;
            vx_float32 t = y_src - y_min;

            vx_bool defined_tl, defined_tr, defined_bl, defined_br;
            if (format == VX_DF_IMAGE_U1)
            {
                defined_tl = read_pixel_1u(src_base, &src_addr, x1 + 0, y1 + 0, borders, &tl, src_rect.start_x % 8);
                defined_tr = read_pixel_1u(src_base, &src_addr, x1 + 1, y1 + 0, borders, &tr, src_rect.start_x % 8);
                defined_bl = read_pixel_1u(src_base, &src_addr, x1 + 0, y1 + 1, borders, &bl, src_rect.start_x % 8);
                defined_br = read_pixel_1u(src_base, &src_addr, x1 + 1, y1 + 1, borders, &br, src_rect.start_x % 8);
            }
            else
            {
                defined_tl = read_pixel_8u(src_base, &src_addr, x1 + 0, y1 + 0, borders, &tl);
                defined_tr = read_pixel_8u(src_base, &src_addr, x1 + 1, y1 + 0, borders, &tr);
                defined_bl = read_pixel_8u(src_base, &src_addr, x1 + 0, y1 + 1, borders, &bl);
                defined_br = read_pixel_8u(src_base, &src_addr, x1 + 1, y1 + 1, borders, &br);
            }
            vx_bool defined = defined_tl & defined_tr & defined_bl & defined_br;
            if (defined == vx_false_e)
            {
                vx_bool defined_any = defined_tl | defined_tr | defined_bl | defined_br;
                if (defined_any)
                {
                    if ((defined_tl == vx_false_e || defined_tr == vx_false_e) && fabs(t - 1.0) <= 0.001)
                        defined_tl = defined_tr = vx_true_e;
                    else if ((defined_bl == vx_false_e || defined_br == vx_false_e) && fabs(t - 0.0) <= 0.001)
                        defined_bl = defined_br = vx_true_e;
                    if ((defined_tl == vx_false_e || defined_bl == vx_false_e) && fabs(s - 1.0) <= 0.001)
                        defined_tl = defined_bl = vx_true_e;
                    else if ((defined_tr == vx_false_e || defined_br == vx_false_e) && fabs(s - 0.0) <= 0.001)
                        defined_tr = defined_br = vx_true_e;
                    defined = defined_tl & defined_tr & defined_bl & defined_br;
                }
            }

            if (defined == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s) * (1 - t) * tl +
                        (    s) * (1 - t) * tr +
                        (1 - s) * (    t) * bl +
                        (    s) * (    t) * br;
                vx_uint8 ref_8u;
                if (format == VX_DF_IMAGE_U1)   // Rounding instead of thresholding for U1 images
                    ref = ref + 0.5f;

                if (format == VX_DF_IMAGE_U8 && ref > 255)
                    ref_8u = 255;
                else if (format == VX_DF_IMAGE_U1 && ref > 1)
                    ref_8u = 1;
                else
                    ref_8u = (vx_uint8)ref;

                if (dst && format == VX_DF_IMAGE_U1)
                    *dst = (*dst & ~(1 << (x2 % 8))) | (ref_8u << (x2 % 8));
                else if (dst && format == VX_DF_IMAGE_U8)
                    *dst = ref_8u;
            }
        }
    }

    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &dst_rect, 0, &dst_addr, dst_base);

    return VX_SUCCESS;
}

#if AREA_SCALE_ENABLE
static vx_status vxAreaScaling(vx_image src_image, vx_image dst_image, const vx_border_t *borders, vx_float64 *interm, vx_size size)
{
    // TODO Implement U1 support for area scaling once the U8 implementation is fixed
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_rectangle_t src_rect, dst_rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint32 by, bx, y, x, yi, xi, gcd_w, gcd_h, r1w, r1h, r2w, r2h, r_w, b_w, b_h;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;

    /*! \bug Should ScaleImage use the valid region of the image
     * as the scaling information or the width,height? If it uses the valid
     * region, should is scale the valid region within bounds of the
     * image?
     */
    vxQueryImage(src_image, VX_IMAGE_WIDTH, &w1, sizeof(w1));
    vxQueryImage(src_image, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
    vxQueryImage(dst_image, VX_IMAGE_WIDTH, &w2, sizeof(w2));
    vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &h2, sizeof(h2));

    src_rect.start_x = src_rect.start_y = 0;
    src_rect.end_x = w1;
    src_rect.end_y = h1;

    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    status = VX_SUCCESS;
    status |= vxAccessImagePatch(src_image, &src_rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &dst_rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    /* compute the Resize block sizes and the intermediate block size */
    gcd_w = math_gcd(w1, w2);
    gcd_h = math_gcd(h1, h2);
    r1w = w1 / gcd_w;
    r2w = w2 / gcd_w;
    r1h = h1 / gcd_h;
    r2h = h2 / gcd_h;
    r_w = r1w * r2w;
    b_w = w1 / r1w;
    b_h = h1 / r1h;

    /*
    {
        vx_uint32 r_h = r1h * r2h;
        printf("%ux%u => %ux%u :: r1:%ux%u => r2:%ux%u (%p:%ux%u) blocks:%ux%u\n",
               w1,h1, w2,h2, r1w,r1h, r2w,r2h, interm, r_w,r_h, b_w,b_h);
    }
    */

    /* iterate over each block */
    for (by = 0; by < b_h; by++)
    {
        for (bx = 0; bx < b_w; bx++)
        {
            /* convert source image to intermediate */
            for (y = 0; y < r1h; y++)
            {
                for (x = 0; x < r1w; x++)
                {
                    vx_uint32 i = (((by * r1h) + y) * src_addr.stride_y) +
                                  (((bx * r1w) + x) * src_addr.stride_x);
                    vx_uint8 pixel = ((vx_uint8 *)src_base)[i];
                    for (yi = 0; yi < r2h; yi++)
                    {
                        for (xi = 0; xi < r2w; xi++)
                        {
                            vx_uint32 k = ((((y * r2h) + yi) * r_w) +
                                            ((x * r2w) + xi));
                            interm[k] = (vx_float64)pixel / (vx_float64)(r2w*r2h);
                        }
                    }
                }
            }

            /* convert the intermediate into the destination */
            for (y = 0; y < r2h; y++)
            {
                for (x = 0; x < r2w; x++)
                {
                    uint32_t sum32 = 0;
                    vx_float64 sum = 0.0f;
                    vx_uint32 i = (((by * r2h) + y) * dst_addr.stride_y) +
                                  (((bx * r2w) + x) * dst_addr.stride_x);
                    vx_uint8 *dst = &((vx_uint8 *)dst_base)[i];
                    /* sum intermediate into destination */
                    for (yi = 0; yi < r1h; yi++)
                    {
                        for (xi = 0; xi < r1w; xi++)
                        {
                            vx_uint32 k = ((((y * r1h) + yi) * r_w) +
                                            ((x * r1w) + xi));
                            sum += interm[k];
                        }
                    }
                    /* rescale the output value */
                    sum32 = (vx_uint32)(sum * (r2w*r2h)/(r1w*r1h));
                    sum32 = (sum32 > 255?255:sum32);
                    *dst = (vx_uint8)sum32;
                }
            }
        }
    }

    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &dst_rect, 0, &dst_addr, dst_base);
    return VX_SUCCESS;
}
#endif

// nodeless version of the ScaleImage kernel
vx_status vxScaleImage(vx_image src_image, vx_image dst_image, vx_scalar stype, vx_border_t *bordermode, vx_float64 *interm, vx_size size)
{
    vx_status status = VX_FAILURE;
    vx_enum type = 0;

    vxCopyScalar(stype, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    if (interm && size)
    {
        if (type == VX_INTERPOLATION_BILINEAR)
        {
            status = vxBilinearScaling(src_image, dst_image, bordermode);
        }
        else if (type == VX_INTERPOLATION_AREA)
        {
#if AREA_SCALE_ENABLE // TODO FIX THIS
            status = vxAreaScaling(src_image, dst_image, bordermode, interm, size);
#else
            status = vxNearestScaling(src_image, dst_image, bordermode);
#endif
        }
        else if (type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
        {
            status = vxNearestScaling(src_image, dst_image, bordermode);
        }
    }
    else
    {
        status = VX_ERROR_NO_RESOURCES;
    }

    return status;
}

