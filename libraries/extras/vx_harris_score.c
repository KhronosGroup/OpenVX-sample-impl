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
 * \brief The Filter Kernel (Extras)
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>

#ifndef OWN_MAX
#define OWN_MAX(a, b) (a) > (b) ? (a) : (b)
#endif

#ifndef OWN_MIN
#define OWN_MIN(a, b) (a) < (b) ? (a) : (b)
#endif

static
vx_param_description_t harris_score_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownHarrisScoreKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(harris_score_kernel_params))
    {
        vx_image  grad_x      = (vx_image)parameters[0];
        vx_image  grad_y      = (vx_image)parameters[1];
        vx_scalar sensitivity = (vx_scalar)parameters[2];
        vx_scalar grad_s      = (vx_scalar)parameters[3];
        vx_scalar winds       = (vx_scalar)parameters[4];
        vx_image  dst         = (vx_image)parameters[5];

        vx_float32 k = 0.0f;
        vx_uint32 block_size = 0;
        vx_uint32 grad_size = 0;
        vx_rectangle_t rect;

        status = vxGetValidRegionImage(grad_x, &rect);

        status |= vxCopyScalar(grad_s, &grad_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar(winds, &block_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar(sensitivity, &k, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

        if (status == VX_SUCCESS)
        {
            vx_int32 x;
            vx_int32 y;
            vx_int32 i;
            vx_int32 j;
            void* gx_base = NULL;
            void* gy_base = NULL;
            void* dst_base = NULL;
            vx_imagepatch_addressing_t gx_addr  = VX_IMAGEPATCH_ADDR_INIT;
            vx_imagepatch_addressing_t gy_addr  = VX_IMAGEPATCH_ADDR_INIT;
            vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
            vx_map_id grad_x_map_id = 0;
            vx_map_id grad_y_map_id = 0;
            vx_map_id dst_map_id = 0;
            vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

            status |= vxMapImagePatch(grad_x, &rect, 0, &grad_x_map_id, &gx_addr, &gx_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
            status |= vxMapImagePatch(grad_y, &rect, 0, &grad_y_map_id, &gy_addr, &gy_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
            status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

            status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

            if (VX_SUCCESS == status)
            {
                /*! \todo implement other Harris Corners border modes */
                if (borders.mode == VX_BORDER_UNDEFINED)
                {
                    double scale = 1.0 / ((1 << (grad_size - 1)) * block_size * 255.0);

                    vx_int32 b  = (block_size / 2) + 1;
                    vx_int32 b2 = (block_size / 2);

                    vxAlterRectangle(&rect, b, b, -b, -b);

                    for (y = b; (y < (vx_int32)(gx_addr.dim_y - b)); y++)
                    {
                        for (x = b; x < (vx_int32)(gx_addr.dim_x - b); x++)
                        {
                            double sum_ix2   = 0.0;
                            double sum_iy2   = 0.0;
                            double sum_ixy   = 0.0;
                            double det_A     = 0.0;
                            double trace_A   = 0.0;
                            double ktrace_A2 = 0.0;
                            double M_c       = 0.0;

                            vx_float32* pmc = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                            for (j = -b2; j <= b2; j++)
                            {
                                for (i = -b2; i <= b2; i++)
                                {
                                    vx_float32* pgx = vxFormatImagePatchAddress2d(gx_base, x + i, y + j, &gx_addr);
                                    vx_float32* pgy = vxFormatImagePatchAddress2d(gy_base, x + i, y + j, &gy_addr);

                                    vx_float32 gx = (*pgx);
                                    vx_float32 gy = (*pgy);

                                    sum_ix2 += gx * gx * scale * scale;
                                    sum_iy2 += gy * gy * scale * scale;
                                    sum_ixy += gx * gy * scale * scale;
                                }
                            }

                            det_A = (sum_ix2 * sum_iy2) - (sum_ixy * sum_ixy);
                            trace_A = sum_ix2 + sum_iy2;
                            ktrace_A2 = (k * (trace_A * trace_A));

                            M_c = det_A - ktrace_A2;

                            *pmc = (vx_float32)M_c;
#if 0
                            if (sum_ix2 > 0 || sum_iy2 > 0 || sum_ixy > 0)
                            {
                                printf("Σx²=%d Σy²=%d Σxy=%d\n",sum_ix2,sum_iy2,sum_ixy);
                                printf("det|A|=%lld trace(A)=%lld M_c=%lld\n",det_A,trace_A,M_c);
                                printf("{x,y,M_c32}={%d,%d,%d}\n",x,y,M_c32);
                            }
#endif
                        }
                    }
                }
                else
                {
                    status = VX_ERROR_NOT_IMPLEMENTED;
                }
            }

            status |= vxUnmapImagePatch(grad_x, grad_x_map_id);
            status |= vxUnmapImagePatch(grad_y, grad_y_map_id);
            status |= vxUnmapImagePatch(dst, dst_map_id);
        }
    } // if ptrs non NULL

    return status;
} /* ownHarrisScoreKernel() */

static
vx_status VX_CALLBACK set_harris_score_valid_rectangle(
    vx_node node,
    vx_uint32 index,
    const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && index < dimof(harris_score_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
        vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

        status = vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
        if (VX_SUCCESS != status)
            return status;

        if (VX_BORDER_UNDEFINED == borders.mode)
        {
            vx_parameter param      = 0;
            vx_scalar    block_size = 0;

            param = vxGetParameterByIndex(node, 4);

            if (VX_SUCCESS == vxGetStatus((vx_reference)param) &&
                VX_SUCCESS == vxQueryParameter(param, VX_PARAMETER_REF, &block_size, sizeof(block_size)))
            {
                vx_enum type = 0;

                status = vxQueryScalar(block_size, VX_SCALAR_TYPE, &type, sizeof(type));

                if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
                {
                    vx_int32 win_size = 0;

                    status = vxCopyScalar(block_size, &win_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                    if (VX_SUCCESS == status &&
                        (win_size == 3 || win_size == 5 || win_size == 7))
                    {
                        vx_uint32 border_size = (win_size / 2) + 1;

                        if (input_valid[0]->start_x > input_valid[1]->end_x ||
                            input_valid[0]->end_x   < input_valid[1]->start_x ||
                            input_valid[0]->start_y > input_valid[1]->end_y ||
                            input_valid[0]->end_y   < input_valid[1]->start_y)
                        {
                            /* no intersection */
                            status = VX_ERROR_INVALID_PARAMETERS;
                        }
                        else
                        {
                            output_valid[0]->start_x = OWN_MAX(input_valid[0]->start_x, input_valid[1]->start_x) + border_size;
                            output_valid[0]->start_y = OWN_MAX(input_valid[0]->start_y, input_valid[1]->start_y) + border_size;
                            output_valid[0]->end_x   = OWN_MIN(input_valid[0]->end_x, input_valid[1]->end_x) - border_size;
                            output_valid[0]->end_y   = OWN_MIN(input_valid[0]->end_y, input_valid[1]->end_y) - border_size;
                            status = VX_SUCCESS;
                        }
                    }
                    else
                        status = VX_ERROR_INVALID_PARAMETERS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }
            else
                status = VX_ERROR_INVALID_PARAMETERS;

            if (NULL != block_size)
                vxReleaseScalar(&block_size);

            if (NULL != param)
                vxReleaseParameter(&param);
        } // if BORDER_UNDEFINED
        else
            status = VX_ERROR_NOT_IMPLEMENTED;
    } // if ptrs non NULL

    return status;
} /* set_harris_score_valid_rectangle() */

static
vx_status VX_CALLBACK own_harris_score_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (NULL != node &&
        num == dimof(harris_score_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;
        vx_parameter param3 = 0;
        vx_parameter param4 = 0;
        vx_parameter param5 = 0;

        vx_image  dx = 0;
        vx_image  dy = 0;
        vx_scalar sensitivity = 0;
        vx_scalar gradient_size = 0;
        vx_scalar block_size = 0;

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);
        param3 = vxGetParameterByIndex(node, 2);
        param4 = vxGetParameterByIndex(node, 3);
        param5 = vxGetParameterByIndex(node, 4);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param3) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param4) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param5))
        {
            status = VX_SUCCESS;

            status |= vxQueryParameter(param1, VX_PARAMETER_REF, &dx, sizeof(dx));
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &dy, sizeof(dy));
            status |= vxQueryParameter(param3, VX_PARAMETER_REF, &sensitivity, sizeof(sensitivity));
            status |= vxQueryParameter(param4, VX_PARAMETER_REF, &gradient_size, sizeof(gradient_size));
            status |= vxQueryParameter(param5, VX_PARAMETER_REF, &block_size, sizeof(block_size));

            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)dx) &&
                VX_SUCCESS == vxGetStatus((vx_reference)dy) &&
                VX_SUCCESS == vxGetStatus((vx_reference)sensitivity) &&
                VX_SUCCESS == vxGetStatus((vx_reference)gradient_size) &&
                VX_SUCCESS == vxGetStatus((vx_reference)block_size))
            {
                vx_uint32   dx_width = 0;
                vx_uint32   dx_height = 0;
                vx_df_image dx_format = 0;

                vx_uint32   dy_width = 0;
                vx_uint32   dy_height = 0;
                vx_df_image dy_format = 0;

                status |= vxQueryImage(dx, VX_IMAGE_WIDTH, &dx_width, sizeof(dx_width));
                status |= vxQueryImage(dx, VX_IMAGE_HEIGHT, &dx_height, sizeof(dx_height));
                status |= vxQueryImage(dx, VX_IMAGE_FORMAT, &dx_format, sizeof(dx_format));

                status |= vxQueryImage(dy, VX_IMAGE_WIDTH, &dy_width, sizeof(dy_width));
                status |= vxQueryImage(dy, VX_IMAGE_HEIGHT, &dy_height, sizeof(dy_height));
                status |= vxQueryImage(dy, VX_IMAGE_FORMAT, &dy_format, sizeof(dy_format));

                /* validate input images */
                if (VX_SUCCESS == status &&
                    dx_format == VX_DF_IMAGE_F32 && dy_format == VX_DF_IMAGE_F32 &&
                    dx_width == dy_width &&
                    dx_height == dy_height)
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_FORMAT;

                /* validate input sensitivity */
                if (VX_SUCCESS == status)
                {
                    vx_enum type = 0;

                    status |= vxQueryScalar(sensitivity, VX_SCALAR_TYPE, &type, sizeof(type));

                    if (VX_SUCCESS == status && VX_TYPE_FLOAT32 == type)
                        status = VX_SUCCESS;
                    else
                        status = VX_ERROR_INVALID_TYPE;
                }

                /* validate input gradient_size */
                if (VX_SUCCESS == status)
                {
                    vx_enum type = 0;

                    status |= vxQueryScalar(gradient_size, VX_SCALAR_TYPE, &type, sizeof(type));

                    if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
                    {
                        vx_int32 size = 0;

                        status |= vxCopyScalar(gradient_size, &size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                        if (VX_SUCCESS == status &&
                            (size == 3 || size == 5 || size == 7))
                        {
                            status = VX_SUCCESS;
                        }
                        else
                            status = VX_ERROR_INVALID_PARAMETERS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_TYPE;
                    }
                }

                /* validate input block_size */
                if (VX_SUCCESS == status)
                {
                    vx_enum type = 0;

                    status |= vxQueryScalar(block_size, VX_SCALAR_TYPE, &type, sizeof(type));

                    if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
                    {
                        vx_int32 size = 0;

                        status |= vxCopyScalar(block_size, &size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                        if (VX_SUCCESS == status &&
                            (size == 3 || size == 5 || size == 7))
                        {
                            status = VX_SUCCESS;
                        }
                        else
                            status = VX_ERROR_INVALID_PARAMETERS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_TYPE;
                    }
                }

                /* validate output image */
                if (VX_SUCCESS == status)
                {
                    vx_df_image dst_format = VX_DF_IMAGE_F32;
                    vx_kernel_image_valid_rectangle_f callback = &set_harris_score_valid_rectangle;

                    status |= vxSetMetaFormatAttribute(metas[5], VX_IMAGE_WIDTH,  &dx_width,   sizeof(dx_width));
                    status |= vxSetMetaFormatAttribute(metas[5], VX_IMAGE_HEIGHT, &dx_height,  sizeof(dx_height));
                    status |= vxSetMetaFormatAttribute(metas[5], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

                    status |= vxSetMetaFormatAttribute(metas[5], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
                }
            }
        }

        if (NULL != dx)
            vxReleaseImage(&dx);

        if (NULL != dy)
            vxReleaseImage(&dy);

        if (NULL != sensitivity)
            vxReleaseScalar(&sensitivity);

        if (NULL != gradient_size)
            vxReleaseScalar(&gradient_size);

        if (NULL != block_size)
            vxReleaseScalar(&block_size);

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);

        if (NULL != param3)
            vxReleaseParameter(&param3);

        if (NULL != param4)
            vxReleaseParameter(&param4);

        if (NULL != param5)
            vxReleaseParameter(&param5);
    } // if ptrs non NULL

    return status;
} /* own_harris_score_validator() */

vx_kernel_description_t harris_score_kernel =
{
    VX_KERNEL_EXTRAS_HARRIS_SCORE,
    "org.khronos.extras.harris_score",
    ownHarrisScoreKernel,
    harris_score_kernel_params, dimof(harris_score_kernel_params),
    own_harris_score_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
