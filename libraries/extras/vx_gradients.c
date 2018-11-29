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

/*!
 * \file
 * \brief The Gradient Kernels (Extras)
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <stdio.h>
#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>


/* scharr 3x3 kernel implementation */
static
vx_param_description_t scharr3x3_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL },
};

static
vx_status VX_CALLBACK ownScharr3x3Kernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(scharr3x3_kernel_params))
    {
        vx_uint32 i;
        vx_uint32 x;
        vx_uint32 y;

        vx_image src    = (vx_image)parameters[0];
        vx_image grad_x = (vx_image)parameters[1];
        vx_image grad_y = (vx_image)parameters[2];

        vx_uint8* src_base   = NULL;
        vx_int16* dst_base_x = NULL;
        vx_int16* dst_base_y = NULL;

        vx_imagepatch_addressing_t src_addr   = VX_IMAGEPATCH_ADDR_INIT;
        vx_imagepatch_addressing_t dst_addr_x = VX_IMAGEPATCH_ADDR_INIT;
        vx_imagepatch_addressing_t dst_addr_y = VX_IMAGEPATCH_ADDR_INIT;

        vx_map_id src_map_id    = 0;
        vx_map_id grad_x_map_id = 0;
        vx_map_id grad_y_map_id = 0;

        vx_int16 ops[] = { 3, 10, 3, -3, -10, -3 };
        vx_rectangle_t rect;
        vx_border_t borders = { VX_BORDER_UNDEFINED, {{ 0 }} };

        if (NULL == src || (NULL == grad_x && NULL == grad_y))
        {
            return VX_ERROR_INVALID_PARAMETERS;
        }

        status = vxGetValidRegionImage(src, &rect);

        status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, (void**)&src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        if (NULL != grad_x)
            status |= vxMapImagePatch(grad_x, &rect, 0, &grad_x_map_id, &dst_addr_x, (void**)&dst_base_x, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        if (NULL != grad_y)
            status |= vxMapImagePatch(grad_y, &rect, 0, &grad_y_map_id, &dst_addr_y, (void**)&dst_base_y, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

        if (VX_SUCCESS == status)
        {
            /*! \todo Implement other border modes */
            if (borders.mode == VX_BORDER_UNDEFINED)
            {
                /* shrink the image by 1 */
                vxAlterRectangle(&rect, 1, 1, -1, -1);

                for (y = 1; y < (src_addr.dim_y - 1); y++)
                {
                    for (x = 1; x < (src_addr.dim_x - 1); x++)
                    {
                        vx_uint8* gdx[] =
                        {
                            vxFormatImagePatchAddress2d(src_base, x + 1, y - 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x + 1, y,     &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x + 1, y + 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x - 1, y - 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x - 1, y,     &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x - 1, y + 1, &src_addr)
                        };

                        vx_uint8* gdy[] =
                        {
                            vxFormatImagePatchAddress2d(src_base, x - 1, y + 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x,     y + 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x + 1, y + 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x - 1, y - 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x,     y - 1, &src_addr),
                            vxFormatImagePatchAddress2d(src_base, x + 1, y - 1, &src_addr)
                        };

                        vx_int16* out_x = vxFormatImagePatchAddress2d(dst_base_x, x, y, &dst_addr_x);
                        vx_int16* out_y = vxFormatImagePatchAddress2d(dst_base_y, x, y, &dst_addr_y);

                        if (out_x)
                        {
                            *out_x = 0;
                            for (i = 0; i < dimof(gdx); i++)
                                *out_x += (ops[i] * gdx[i][0]);
                        }

                        if (out_y)
                        {
                            *out_y = 0;
                            for (i = 0; i < dimof(gdy); i++)
                                *out_y += (ops[i] * gdy[i][0]);
                        }
                    } // for x
                } // for y
            } // VX_BORDER_UNDEFINED
            else
            {
                status = VX_ERROR_NOT_IMPLEMENTED;
            }
        }

        status |= vxUnmapImagePatch(src, src_map_id);

        if (NULL != grad_x)
            status |= vxUnmapImagePatch(grad_x, grad_x_map_id);

        if (NULL != grad_y)
            status |= vxUnmapImagePatch(grad_y, grad_y_map_id);
    } /* if ptrs non NULL */

    return status;
} /* ownScharr3x3Kernel() */

static
vx_status VX_CALLBACK set_scharr3x3_valid_rectangle(
    vx_node node,
    vx_uint32 index,
    const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && index < dimof(scharr3x3_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
        vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

        if (VX_SUCCESS == vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders)))
        {
            if (VX_BORDER_UNDEFINED == borders.mode)
            {
                vx_uint32 border_size = 1;

                output_valid[0]->start_x = input_valid[0]->start_x + border_size;
                output_valid[0]->start_y = input_valid[0]->start_y + border_size;
                output_valid[0]->end_x   = input_valid[0]->end_x   - border_size;
                output_valid[0]->end_y   = input_valid[0]->end_y   - border_size;
                status = VX_SUCCESS;
            }
            else
                status = VX_ERROR_NOT_IMPLEMENTED;
        }
    } // if ptrs non NULL

    return status;
} /* set_scharr3x3_valid_rectangle() */

static
vx_status VX_CALLBACK own_scharr3x3_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (NULL != node &&
        num == dimof(scharr3x3_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param = 0;
        vx_image     src = 0;

        param = vxGetParameterByIndex(node, 0);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param))
        {
            status = vxQueryParameter(param, VX_PARAMETER_REF, &src, sizeof(src));

            if (VX_SUCCESS == status)
            {
                vx_uint32   src_width  = 0;
                vx_uint32   src_height = 0;
                vx_df_image src_format = 0;

                status |= vxQueryImage(src, VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                status |= vxQueryImage(src, VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));

                /* validate input image */
                if (VX_SUCCESS == status &&
                    (src_width >= 3 && src_height >= 3 && src_format == VX_DF_IMAGE_U8))
                {
                    status = VX_SUCCESS;
                }

                /* validate output images */
                if (VX_SUCCESS == status)
                {
                    vx_enum dst_format = VX_DF_IMAGE_S16;
                    vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

                    if (NULL != metas[1])
                    {
                        /* if optional parameter non NULL */
                        status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_WIDTH, &src_width, sizeof(src_width));
                        status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                        status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                    }

                    if (NULL != metas[2])
                    {
                        /* if optional parameter non NULL */
                        status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_WIDTH, &src_width, sizeof(src_width));
                        status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                        status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                    }

                    status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

                    if (VX_SUCCESS == status)
                    {
                        if (VX_BORDER_UNDEFINED == borders.mode)
                        {
                            vx_kernel_image_valid_rectangle_f callback = &set_scharr3x3_valid_rectangle;

                            if (NULL != metas[1])
                                status |= vxSetMetaFormatAttribute(metas[1], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));

                            if (NULL != metas[2])
                                status |= vxSetMetaFormatAttribute(metas[2], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
                        }
                        else
                            status = VX_ERROR_NOT_IMPLEMENTED;
                    }
                }
            }
        }
        else
            status = VX_ERROR_INVALID_PARAMETERS;

        if (NULL != src)
            vxReleaseImage(&src);

        if (NULL != param)
            vxReleaseParameter(&param);
    } // if ptrs non NULL

    return status;
} /* own_scharr3x3_validator() */

vx_kernel_description_t scharr3x3_kernel =
{
    VX_KERNEL_EXTRAS_SCHARR_3x3,
    "org.khronos.extras.scharr3x3",
    ownScharr3x3Kernel,
    scharr3x3_kernel_params, dimof(scharr3x3_kernel_params),
    own_scharr3x3_validator,
    NULL,
    NULL,
    NULL,
    NULL
};


/* Sobel 3x3, 5x5, 7x7 kernel implementation */

static
vx_param_description_t sobelMxN_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_OPTIONAL },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_OPTIONAL },
};


/* 3x3 kernels (x and y) */
static
vx_int16 ops3_x[3][3] =
{
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1},
};

static
vx_int16 ops3_y[3][3] =
{
    {-1,-2,-1},
    { 0, 0, 0},
    { 1, 2, 1},
};

/* 5x5 kernels (x and y) */
static
vx_int16 ops5_x[5][5] =
{
    {-1, -2, 0, 2, 1},
    {-4, -8, 0, 8, 4},
    {-6,-12, 0,12, 6},
    {-4, -8, 0, 8, 4},
    {-1, -2, 0, 2, 1},
};

static
vx_int16 ops5_y[5][5] =
{
    {-1,-4, -6,-4,-1},
    {-2,-8,-12,-8,-2},
    { 0, 0,  0, 0, 0},
    { 2, 8, 12, 8, 2},
    { 1, 4,  6, 4, 1},
};

/* 7x7 kernels (x and y) */
static
vx_int16 ops7_x[7][7] =
{
    { -1,  -4,  -5, 0,   5,  4,  1},
    { -6, -24, -30, 0,  30, 24,  6},
    {-15, -60, -75, 0,  75, 60, 15},
    {-20, -80,-100, 0, 100, 80, 20},
    {-15, -60, -75, 0,  75, 60, 15},
    { -6, -24, -30, 0,  30, 24,  6},
    { -1,  -4,  -5, 0,   5,  4,  1},
};

static
vx_int16 ops7_y[7][7] =
{
    {-1, -6,-15, -20,-15, -6,-1},
    {-4,-24,-60, -80,-60,-24,-4},
    {-5,-30,-75,-100,-75,-30,-5},
    { 0,  0,  0,   0,  0,  0, 0},
    { 5, 30, 75, 100, 75, 30, 5},
    { 4, 24, 60,  80, 60, 24, 4},
    { 1,  6, 15,  20, 15,  6, 1},
};

static
vx_status VX_CALLBACK ownSobelMxNKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(sobelMxN_kernel_params))
    {
        vx_uint32 x;
        vx_uint32 y;
        vx_int32 ws = 0;
        vx_int32 b;
        vx_uint32 low_x;
        vx_uint32 high_x;
        vx_uint32 low_y;
        vx_uint32 high_y;
        vx_uint8*   src_base = NULL;
        vx_float32* dst_base_x = NULL;
        vx_float32* dst_base_y = NULL;
        vx_int16*   opx = NULL;
        vx_int16*   opy = NULL;
        vx_imagepatch_addressing_t src_addr   = VX_IMAGEPATCH_ADDR_INIT;
        vx_imagepatch_addressing_t dst_addr_x = VX_IMAGEPATCH_ADDR_INIT;
        vx_imagepatch_addressing_t dst_addr_y = VX_IMAGEPATCH_ADDR_INIT;
        vx_map_id src_map_id = 0;
        vx_map_id grad_x_map_id = 0;
        vx_map_id grad_y_map_id = 0;
        vx_rectangle_t rect;
        vx_border_t borders = { VX_BORDER_UNDEFINED, {{ 0 }} };

        vx_image  src    = (vx_image)parameters[0];
        vx_scalar win    = (vx_scalar)parameters[1];
        vx_image  grad_x = (vx_image)parameters[2];
        vx_image  grad_y = (vx_image)parameters[3];

        if (NULL == src || (NULL == grad_x && NULL == grad_y))
        {
            return VX_ERROR_INVALID_PARAMETERS;
        }

        status = vxCopyScalar(win, &ws, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        b = ws/2;

        if (ws == 3)
        {
            opx = &ops3_x[0][0];
            opy = &ops3_y[0][0];
        }
        else if (ws == 5)
        {
            opx = &ops5_x[0][0];
            opy = &ops5_y[0][0];
        }
        else if (ws == 7)
        {
            opx = &ops7_x[0][0];
            opy = &ops7_y[0][0];
        }

        status |= vxGetValidRegionImage(src, &rect);

        status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, (void**)&src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        if (NULL != grad_x)
            status |= vxMapImagePatch(grad_x, &rect, 0, &grad_x_map_id, &dst_addr_x, (void**)&dst_base_x, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        if (NULL != grad_y)
            status |= vxMapImagePatch(grad_y, &rect, 0, &grad_y_map_id, &dst_addr_y, (void**)&dst_base_y, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

        low_x  = 0;
        low_y  = 0;
        high_x = src_addr.dim_x;
        high_y = src_addr.dim_y;

        if (borders.mode == VX_BORDER_UNDEFINED)
        {
            low_x += b; high_x -= b;
            low_y += b; high_y -= b;
            status |= vxAlterRectangle(&rect, b, b, -b, -b);
        }

        if (VX_SUCCESS == status)
        {
            for (y = low_y; y < high_y; y++)
            {
                for (x = low_x; x < high_x; x++)
                {
                    vx_int32 i;
                    vx_int32 sum_x = 0;
                    vx_int32 sum_y = 0;
                    vx_float32* out_x = 0;
                    vx_float32* out_y = 0;

                    vx_uint8 square[7 * 7];
                    vxReadRectangle(src_base, &src_addr, &borders, VX_DF_IMAGE_U8, x, y, b, b, square);

                    for (i = 0; i < ws * ws; ++i)
                    {
                        sum_x += opx[i] * square[i];
                        sum_y += opy[i] * square[i];
                    }

                    if (NULL != dst_base_x)
                        out_x = vxFormatImagePatchAddress2d(dst_base_x, x, y, &dst_addr_x);

                    if (NULL != dst_base_y)
                        out_y = vxFormatImagePatchAddress2d(dst_base_y, x, y, &dst_addr_y);

                    if (NULL != out_x)
                        *out_x = (vx_float32)sum_x;

                    if (NULL != out_y)
                        *out_y = (vx_float32)sum_y;
                } // for x
            } // for y
        }

        status |= vxUnmapImagePatch(src, src_map_id);
        status |= vxUnmapImagePatch(grad_x, grad_x_map_id);
        status |= vxUnmapImagePatch(grad_y, grad_y_map_id);
    } // if ptrs non NULL

    return status;
} /* ownSobelMxNKernel() */

static
vx_status VX_CALLBACK set_sobelMxN_valid_rectangle(
    vx_node node,
    vx_uint32 index,
    const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_FAILURE;

    if (NULL != node && index < dimof(sobelMxN_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
        vx_parameter   param = 0;
        vx_scalar      win = 0;
        vx_enum        type = VX_TYPE_INVALID;
        vx_int32       kernel_size = 0;
        vx_border_t    borders = { VX_BORDER_UNDEFINED, { { 0 } } };

        param = vxGetParameterByIndex(node, 1);
        if (VX_SUCCESS != vxGetStatus((vx_reference)param))
            return VX_ERROR_INVALID_PARAMETERS;

        status = vxQueryParameter(param, VX_PARAMETER_REF, &win, sizeof(win));

        if (VX_SUCCESS == status)
        {
            status = vxQueryScalar(win, VX_SCALAR_TYPE, &type, sizeof(type));
            if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
            {
                status = vxCopyScalar(win, &kernel_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                if (VX_SUCCESS == status &&
                    (kernel_size == 3 || kernel_size == 5 || kernel_size == 7))
                {
                    status = vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

                    if (VX_SUCCESS == status)
                    {
                        vx_uint32 border_size = 0;

                        if (VX_BORDER_UNDEFINED == borders.mode)
                        {
                            border_size = kernel_size / 2;
                        }

                        output_valid[0]->start_x = input_valid[0]->start_x + border_size;
                        output_valid[0]->start_y = input_valid[0]->start_y + border_size;
                        output_valid[0]->end_x   = input_valid[0]->end_x   - border_size;
                        output_valid[0]->end_y   = input_valid[0]->end_y   - border_size;

                        status = VX_SUCCESS;
                    }
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }
            else
                status = VX_ERROR_INVALID_PARAMETERS;
        }

        if (NULL != win)
            vxReleaseScalar(&win);

        if (NULL != param)
            vxReleaseParameter(&param);
    } // if ptrs non NULL

    return status;
} /* set_sobelMxN_valid_rectangle() */

static
vx_status VX_CALLBACK own_sobelMxN_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (NULL != node &&
        num == dimof(sobelMxN_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;

        vx_image  src = 0; /* param1 */
        vx_scalar win = 0; /* param2 */

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);

        if (VX_SUCCESS != vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS != vxGetStatus((vx_reference)param2))
        {
            return VX_ERROR_INVALID_PARAMETERS;
        }

        status = VX_SUCCESS;

        status |= vxQueryParameter(param1, VX_PARAMETER_REF, &src, sizeof(src));
        status |= vxQueryParameter(param2, VX_PARAMETER_REF, &win, sizeof(win));

        if (VX_SUCCESS == status)
        {
            vx_uint32   src_width   = 0;
            vx_uint32   src_height  = 0;
            vx_df_image src_format  = 0;
            vx_enum     type        = VX_TYPE_INVALID;
            vx_uint32   kernel_size = 0;

            /* validate filter kernel size */
            status = vxQueryScalar(win, VX_SCALAR_TYPE, &type, sizeof(type));
            if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
            {
                status = vxCopyScalar(win, &kernel_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                if (VX_SUCCESS == status &&
                    (kernel_size == 3 || kernel_size == 5 || kernel_size == 7))
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }
            else
                status = VX_ERROR_INVALID_PARAMETERS;

            /* validate input image */
            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)src))
            {
                status |= vxQueryImage(src, VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                status |= vxQueryImage(src, VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));

                /* expected VX_DF_IMAGE_U8 format and size not less than filter kernel size */
                if (VX_SUCCESS == status &&
                    src_width >= kernel_size && src_height >= kernel_size && src_format == VX_DF_IMAGE_U8)
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }
            else
                status = VX_ERROR_INVALID_PARAMETERS;

            /* validate output images */
            if (VX_SUCCESS == status)
            {
                /* sobel MxN use F32 as output format to maintain arithmetic precision for 5x5 and 7x7 kernels */
                vx_df_image dst_format = VX_DF_IMAGE_F32;
                vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

                if (NULL != metas[2])
                {
                    /* if optional parameter non NULL */
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_WIDTH, &src_width, sizeof(src_width));
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                }

                if (NULL != metas[3])
                {
                    /* if optional parameter non NULL */
                    status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_WIDTH, &src_width, sizeof(src_width));
                    status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                    status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                }

                status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

                if (VX_SUCCESS == status && VX_BORDER_UNDEFINED == borders.mode)
                {
                    vx_kernel_image_valid_rectangle_f callback = &set_sobelMxN_valid_rectangle;

                    if (NULL != metas[2])
                        status |= vxSetMetaFormatAttribute(metas[2], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));

                    if (NULL != metas[3])
                        status |= vxSetMetaFormatAttribute(metas[3], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
                }
            }
        }
        else
            status = VX_ERROR_INVALID_PARAMETERS;

        if (NULL != src)
            vxReleaseImage(&src);

        if (NULL != win)
            vxReleaseScalar(&win);

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);
    } /* if input ptrs != NULL */

    return status;
} /* own_sobelMxN_validator() */

vx_kernel_description_t sobelMxN_kernel =
{
    VX_KERNEL_EXTRAS_SOBEL_MxN,
    "org.khronos.extras.sobelMxN",
    ownSobelMxNKernel,
    sobelMxN_kernel_params, dimof(sobelMxN_kernel_params),
    own_sobelMxN_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};

