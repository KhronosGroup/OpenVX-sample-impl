/*

 * Copyright (c) 2016-2017 The Khronos Group Inc.
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


// helpers
static int vx_uint8_compare(const void *p1, const void *p2)
{
    vx_uint8 a = *(vx_uint8 *)p1;
    vx_uint8 b = *(vx_uint8 *)p2;
    if (a > b)
        return 1;
    else if (a == b)
        return 0;
    else
        return -1;
}

static vx_uint32 readMaskedRectangle_U8(const void *base,
    const vx_imagepatch_addressing_t *addr,
    const vx_border_t *borders,
    vx_df_image type,
    vx_uint32 center_x,
    vx_uint32 center_y,
    vx_uint32 left,
    vx_uint32 top,
    vx_uint32 right,
    vx_uint32 bottom,
    vx_uint8 *mask,
    vx_uint8 *destination)
{
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 mask_index = 0;
    vx_uint32 dest_index = 0;
    (void)type;

    // kx, kx - kernel x and y
    if (borders->mode == VX_BORDER_REPLICATE || borders->mode == VX_BORDER_UNDEFINED)
    {
        for (ky = -(int32_t)top; ky <= (int32_t)bottom; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            y = y < 0 ? 0 : y >= height ? height - 1 : y;

            for (kx = -(int32_t)left; kx <= (int32_t)right; ++kx, ++mask_index)
            {
                vx_int32 x = (int32_t)(center_x + kx);
                x = x < 0 ? 0 : x >= width ? width - 1 : x;
                if (mask[mask_index])
                    ((vx_uint8*)destination)[dest_index++] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
            }
        }
    }
    else if (borders->mode == VX_BORDER_CONSTANT)
    {
        vx_pixel_value_t cval = borders->constant_value;
        for (ky = -(int32_t)top; ky <= (int32_t)bottom; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            int ccase_y = y < 0 || y >= height;

            for (kx = -(int32_t)left; kx <= (int32_t)right; ++kx, ++mask_index)
            {
                vx_int32 x = (int32_t)(center_x + kx);
                int ccase = ccase_y || x < 0 || x >= width;
                if (mask[mask_index])
                    ((vx_uint8*)destination)[dest_index++] = ccase ? (vx_uint8)cval.U8 : *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
            }
        }
    }

    return dest_index;
}


// nodeless version of NonLinearFilter kernel
vx_status vxNonLinearFilter(vx_scalar function, vx_image src, vx_matrix mask, vx_image dst, vx_border_t *border)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y;

    vx_uint8 m[C_MAX_NONLINEAR_DIM * C_MAX_NONLINEAR_DIM];
    vx_uint8 v[C_MAX_NONLINEAR_DIM * C_MAX_NONLINEAR_DIM];

    vx_status status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    vx_enum func = 0;
    status |= vxCopyScalar(function, &func, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    vx_size mrows, mcols;
    vx_enum mtype = 0;
    status |= vxQueryMatrix(mask, VX_MATRIX_ROWS, &mrows, sizeof(mrows));
    status |= vxQueryMatrix(mask, VX_MATRIX_COLUMNS, &mcols, sizeof(mcols));
    status |= vxQueryMatrix(mask, VX_MATRIX_TYPE, &mtype, sizeof(mtype));

    vx_coordinates2d_t origin;
    status |= vxQueryMatrix(mask, VX_MATRIX_ORIGIN, &origin, sizeof(origin));

    if ((mtype != VX_TYPE_UINT8) || (sizeof(m) < mrows * mcols))
        status = VX_ERROR_INVALID_PARAMETERS;

    status |= vxCopyMatrix(mask, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if (status == VX_SUCCESS)
    {
        vx_size rx0 = origin.x;
        vx_size ry0 = origin.y;
        vx_size rx1 = mcols - origin.x - 1;
        vx_size ry1 = mrows - origin.y - 1;

        high_x = src_addr.dim_x;
        high_y = src_addr.dim_y;

        if (border->mode == VX_BORDER_UNDEFINED)
        {
            low_x  += (vx_uint32)rx0;
            low_y  += (vx_uint32)ry0;
            high_x -= (vx_uint32)rx1;
            high_y -= (vx_uint32)ry1;
            vxAlterRectangle(&rect, (vx_int32)rx0, (vx_int32)ry0, -(vx_int32)rx1, -(vx_int32)ry1);
        }

        for (y = low_y; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x++)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_int32 count = (vx_int32)readMaskedRectangle_U8(src_base, &src_addr, border, VX_DF_IMAGE_U8, x, y, (vx_uint32)rx0, (vx_uint32)ry0, (vx_uint32)rx1, (vx_uint32)ry1, m, v);

                qsort(v, count, sizeof(vx_uint8), vx_uint8_compare);

                switch (func)
                {
                case VX_NONLINEAR_FILTER_MIN: *dst = v[0]; break; /* minimal value */
                case VX_NONLINEAR_FILTER_MAX: *dst = v[count - 1]; break; /* maximum value */
                case VX_NONLINEAR_FILTER_MEDIAN: *dst = v[count / 2]; break; /* pick the middle value */
                }
            }
        }
    }

    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

    return status;
}
