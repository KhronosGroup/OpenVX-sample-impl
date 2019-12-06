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

static vx_bool read_pixel_1u_C1(void *base, vx_imagepatch_addressing_t *addr, vx_float32 x, vx_float32 y,
                                const vx_border_t *borders, vx_uint8 *pxl_group, vx_uint32 x_dst, vx_uint32 shift_x_u1)
{
    vx_bool out_of_bounds = (x < shift_x_u1 || y < 0 || x >= addr->dim_x || y >= addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 bpixel;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pxl_group = (*pxl_group & ~(1 << (x_dst % 8))) | ((borders->constant_value.U1 ? 1 : 0) << (x_dst % 8));
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < shift_x_u1 ? shift_x_u1 : x >= addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0          ? 0          : y >= addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (*(vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr) & (1 << (bx % 8))) >> (bx % 8);
    *pxl_group = (*pxl_group & ~(1 << (x_dst % 8))) | (bpixel << (x_dst % 8));

    return vx_true_e;
}

static vx_bool read_pixel_8u_C1(void *base, vx_imagepatch_addressing_t *addr,
                          vx_float32 x, vx_float32 y, const vx_border_t *borders, vx_uint8 *pixel)
{
    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= addr->dim_x || y >= addr->dim_y);
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
    bx = x < 0 ? 0 : x >= addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = *bpixel;

    return vx_true_e;
}

typedef void (*transform_f)(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y);

static void transform_affine(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y)
{
    *src_x = dst_x * m[0] + dst_y * m[2] + m[4];
    *src_y = dst_x * m[1] + dst_y * m[3] + m[5];
}

static void transform_perspective(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y)
{
    vx_float32 z = dst_x * m[2] + dst_y * m[5] + m[8];

    *src_x = (dst_x * m[0] + dst_y * m[3] + m[6]) / z;
    *src_y = (dst_x * m[1] + dst_y * m[4] + m[7]) / z;
}

static vx_status vxWarpGeneric(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image,
                               const vx_border_t *borders, transform_f transform)
{
    vx_status status = VX_SUCCESS;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_uint32 dst_width;
    vx_uint32 dst_height;
    vx_rectangle_t src_rect;
    vx_rectangle_t dst_rect;
    vx_df_image format;

    vx_float32 m[9];
    vx_enum type = 0;

    vx_uint32 x = 0u;
    vx_uint32 y = 0u;

    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;

    status |= vxQueryImage(dst_image, VX_IMAGE_WIDTH, &dst_width, sizeof(dst_width));
    status |= vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &dst_height, sizeof(dst_height));
    status |= vxQueryImage(dst_image, VX_IMAGE_FORMAT, &format, sizeof(format));

    status |= vxGetValidRegionImage(src_image, &src_rect);
    vx_uint32 shift_x_u1 = src_rect.start_x % 8;  // Bit-shift offset for U1 images

    dst_rect.start_x = 0;
    dst_rect.start_y = 0;
    dst_rect.end_x   = dst_width;
    dst_rect.end_y   = dst_height;

    status |= vxMapImagePatch(src_image, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst_image, &dst_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    status |= vxCopyMatrix(matrix, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(stype, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if (status == VX_SUCCESS)
    {
        for (y = 0u; y < dst_addr.dim_y; y++)
        {
            for (x = 0u; x < dst_addr.dim_x; x++)
            {
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                vx_float32 xf;
                vx_float32 yf;
                transform(x, y, m, &xf, &yf);
                xf -= (vx_float32)src_rect.start_x;
                yf -= (vx_float32)src_rect.start_y;
                if (format == VX_DF_IMAGE_U1)   // Add bit-shift offset
                    xf += shift_x_u1;

                if (type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
                {
                    if (format == VX_DF_IMAGE_U1)
                        read_pixel_1u_C1(src_base, &src_addr, xf, yf, borders, dst, x, shift_x_u1);
                    else
                        read_pixel_8u_C1(src_base, &src_addr, xf, yf, borders, dst);
                }
                else if (type == VX_INTERPOLATION_BILINEAR)
                {
                    vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;
                    vx_bool defined = vx_true_e;
                    if (format == VX_DF_IMAGE_U1)
                    {
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf), floorf(yf), borders, &tl, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf), borders, &tr, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf), floorf(yf) + 1, borders, &bl, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf) + 1, borders, &br, 0, shift_x_u1);
                    }
                    else
                    {
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf), floorf(yf), borders, &tl);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf), borders, &tr);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf), floorf(yf) + 1, borders, &bl);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf) + 1, borders, &br);
                    }

                    if (defined)
                    {
                        vx_float32 ar = xf - floorf(xf);
                        vx_float32 ab = yf - floorf(yf);
                        vx_float32 al = 1.0f - ar;
                        vx_float32 at = 1.0f - ab;

                        if (format == VX_DF_IMAGE_U1)
                        {
                            // Arithmetic rounding instead of truncation for U1 images
                            vx_uint8 dst_val = (vx_uint8)(tl * al * at + tr * ar * at + bl * al * ab + br * ar * ab + 0.5);
                            *dst = (*dst & ~(1 << (x % 8))) | (dst_val << (x % 8));
                        }
                        else
                        {
                            *dst = (vx_uint8)(tl * al * at + tr * ar * at + bl * al * ab + br * ar * ab);
                        }
                    }
                }
            }
        }

        /*! \todo compute maximum area rectangle */
    }

    status |= vxCopyMatrix(matrix, m, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxUnmapImagePatch(src_image, src_map_id);
    status |= vxUnmapImagePatch(dst_image, dst_map_id);

    return status;
}

// nodeless version of the WarpAffine kernel
vx_status vxWarpAffine(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders)
{
    return vxWarpGeneric(src_image, matrix, stype, dst_image, borders, transform_affine);
}

// nodeless version of the WarpPerspective kernel
vx_status vxWarpPerspective(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders)
{
    return vxWarpGeneric(src_image, matrix, stype, dst_image, borders, transform_perspective);
}
