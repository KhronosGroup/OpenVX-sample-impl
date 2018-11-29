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
 * \brief Elementwise binary norm kernel (Extras)
 */

#include <math.h>
#include <stdlib.h>
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
vx_status ownNorm(vx_image input_x, vx_image input_y, vx_scalar norm_type, vx_image output)
{
    vx_uint32 x;
    vx_uint32 y;
    vx_df_image format = 0;
    vx_float32* dst_base   = NULL;
    vx_float32* src_base_x = NULL;
    vx_float32* src_base_y = NULL;
    vx_imagepatch_addressing_t src_addr_x = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t src_addr_y = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr   = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id src_x_map_id = 0;
    vx_map_id src_y_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_rectangle_t rect;
    vx_enum norm_type_value;
    vx_status status = VX_SUCCESS;

    status |= vxGetValidRegionImage(input_x, &rect);

    status |= vxMapImagePatch(input_x, &rect, 0, &src_x_map_id, &src_addr_x, (void**)&src_base_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(input_y, &rect, 0, &src_y_map_id, &src_addr_y, (void**)&src_base_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(output, &rect, 0, &dst_map_id, &dst_addr, (void**)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

    status |= vxCopyScalar(norm_type, &norm_type_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxQueryImage(output, VX_IMAGE_FORMAT, &format, sizeof(format));

    for (y = 0; y < src_addr_x.dim_y; y++)
    {
        for (x = 0; x < src_addr_x.dim_x; x++)
        {
            vx_float32* in_x = vxFormatImagePatchAddress2d(src_base_x, x, y, &src_addr_x);
            vx_float32* in_y = vxFormatImagePatchAddress2d(src_base_y, x, y, &src_addr_y);
            vx_float32* dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            vx_float32 value;

            if (norm_type_value == VX_NORM_L1)
            {
                value = fabsf(in_x[0]) + fabsf(in_y[0]);
            }
            else
            {
                value = rintf(hypotf(in_x[0], in_y[0]));
            }

            *dst = value;
        }
    }

    status |= vxUnmapImagePatch(input_x, src_x_map_id);
    status |= vxUnmapImagePatch(input_y, src_y_map_id);
    status |= vxUnmapImagePatch(output, dst_map_id);

    return status;
} /* ownNorm() */

static
vx_param_description_t norm_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownNormKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (NULL != node && NULL != parameters && num == dimof(norm_kernel_params))
    {
        vx_image  input_x   = (vx_image)parameters[0];
        vx_image  input_y   = (vx_image)parameters[1];
        vx_scalar norm_type = (vx_scalar)parameters[2];
        vx_image  output    = (vx_image)parameters[3];

        return ownNorm(input_x, input_y, norm_type, output);
    } // if ptrs non NULL

    return VX_ERROR_INVALID_PARAMETERS;
} /* ownNormKernel() */

static
vx_status VX_CALLBACK set_norm_valid_rectangle(
    vx_node node,
    vx_uint32 index,
    const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && index < dimof(norm_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
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
            output_valid[0]->start_x = OWN_MAX(input_valid[0]->start_x, input_valid[1]->start_x);
            output_valid[0]->start_y = OWN_MAX(input_valid[0]->start_y, input_valid[1]->start_y);
            output_valid[0]->end_x   = OWN_MIN(input_valid[0]->end_x, input_valid[1]->end_x);
            output_valid[0]->end_y   = OWN_MIN(input_valid[0]->end_y, input_valid[1]->end_y);
            status = VX_SUCCESS;
        }
    }

    return status;
} /* set_norm_valid_rectangle() */

static
vx_status VX_CALLBACK own_norm_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (NULL != node &&
        num == dimof(norm_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;
        vx_parameter param3 = 0;

        vx_image  dx = 0;
        vx_image  dy = 0;
        vx_scalar norm_type = 0;

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);
        param3 = vxGetParameterByIndex(node, 2);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param3))
        {
            status = VX_SUCCESS;

            status |= vxQueryParameter(param1, VX_PARAMETER_REF, &dx, sizeof(dx));
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &dy, sizeof(dy));
            status |= vxQueryParameter(param3, VX_PARAMETER_REF, &norm_type, sizeof(norm_type));

            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)dx) &&
                VX_SUCCESS == vxGetStatus((vx_reference)dy) &&
                VX_SUCCESS == vxGetStatus((vx_reference)norm_type))
            {
                vx_uint32   dx_width  = 0;
                vx_uint32   dx_height = 0;
                vx_df_image dx_format = 0;

                vx_uint32   dy_width  = 0;
                vx_uint32   dy_height = 0;
                vx_df_image dy_format = 0;

                vx_enum type = 0;

                status |= vxQueryImage(dx, VX_IMAGE_WIDTH,  &dx_width,  sizeof(dx_width));
                status |= vxQueryImage(dx, VX_IMAGE_HEIGHT, &dx_height, sizeof(dx_height));
                status |= vxQueryImage(dx, VX_IMAGE_FORMAT, &dx_format, sizeof(dx_format));

                status |= vxQueryImage(dy, VX_IMAGE_WIDTH,  &dy_width,  sizeof(dy_width));
                status |= vxQueryImage(dy, VX_IMAGE_HEIGHT, &dy_height, sizeof(dy_height));
                status |= vxQueryImage(dy, VX_IMAGE_FORMAT, &dy_format, sizeof(dy_format));

                status |= vxQueryScalar(norm_type, VX_SCALAR_TYPE, &type, sizeof(type));

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

                /* validate input scalar */
                if (VX_SUCCESS == status && VX_TYPE_ENUM == type)
                {
                    vx_enum value = 0;
                    status |= vxCopyScalar(norm_type, &value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if (VX_SUCCESS == status &&
                        (VX_NORM_L1 == value || VX_NORM_L2 == value))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                        status = VX_ERROR_INVALID_PARAMETERS;
                }

                /* validate output image */
                if (VX_SUCCESS == status)
                {
                    vx_df_image dst_format = VX_DF_IMAGE_F32;
                    vx_kernel_image_valid_rectangle_f callback = &set_norm_valid_rectangle;

                    status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_WIDTH,  &dx_width,   sizeof(dx_width));
                    status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_HEIGHT, &dx_height,  sizeof(dx_height));
                    status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

                    status |= vxSetMetaFormatAttribute(metas[3], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
                }
            }
        }

        if (NULL != norm_type)
            vxReleaseScalar(&norm_type);

        if (NULL != dx)
            vxReleaseImage(&dx);

        if (NULL != dy)
            vxReleaseImage(&dy);

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);

        if (NULL != param3)
            vxReleaseParameter(&param3);
    } // if ptrs non NULL

    return status;
} /* own_norm_validator() */

vx_kernel_description_t norm_kernel =
{
    VX_KERNEL_EXTRAS_ELEMENTWISE_NORM,
    "org.khronos.extra.elementwise_norm",
    ownNormKernel,
    norm_kernel_params, dimof(norm_kernel_params),
    own_norm_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
