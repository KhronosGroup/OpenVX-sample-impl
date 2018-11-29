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

static
vx_param_description_t image_lister_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_ARRAY,  VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL },
};

static
vx_status VX_CALLBACK ownImageListerKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(image_lister_kernel_params))
    {
        vx_uint32 x;
        vx_uint32 y;
        vx_image src = (vx_image)parameters[0];
        vx_array arr = (vx_array)parameters[1];
        vx_scalar s_num_points = (vx_scalar)parameters[2];
        void* src_base = NULL;
        vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
        vx_map_id src_map_id = 0;
        vx_rectangle_t rect;
        vx_df_image format;
        vx_size num_corners = 0;
        vx_size dst_capacity = 0;

        status = vxGetValidRegionImage(src, &rect);

        /* remove any pre-existing points */
        status |= vxTruncateArray(arr, 0);

        status |= vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
        status |= vxQueryArray(arr, VX_ARRAY_CAPACITY, &dst_capacity, sizeof(dst_capacity));

        status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        for (y = 0; y < src_addr.dim_y; y++)
        {
            for (x = 0; x < src_addr.dim_x; x++)
            {
                void* ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                if (ptr)
                {
                    vx_bool set = vx_false_e;
                    vx_float32 strength = 0.0f;
                    if (format == VX_DF_IMAGE_U8)
                    {
                        vx_uint8 pixel = *(vx_uint8 *)ptr;
                        strength = (vx_float32)pixel;
                        set = vx_true_e;
                    }
                    else if (format == VX_DF_IMAGE_S16)
                    {
                        vx_int16 pixel = *(vx_int16 *)ptr;
                        strength = (vx_float32)pixel;
                        set = vx_true_e;
                    }
                    else if (format == VX_DF_IMAGE_S32)
                    {
                        vx_int32 pixel = *(vx_int32 *)ptr;
                        strength = (vx_float32)pixel;
                        set = vx_true_e;
                    }
                    else if (format == VX_DF_IMAGE_F32)
                    {
                        vx_float32 pixel = *(vx_float32 *)ptr;
                        strength = pixel;
                        set = vx_true_e;
                    }

                    if ((set == vx_true_e) && (strength > 0.0f))
                    {
                        if (num_corners < dst_capacity)
                        {
                            vx_keypoint_t keypoint;

                            keypoint.x               = rect.start_x + x;
                            keypoint.y               = rect.start_y + y;
                            keypoint.strength        = strength;
                            keypoint.scale           = 0.0f;
                            keypoint.orientation     = 0.0f;
                            keypoint.tracking_status = 1;
                            keypoint.error           = 0.0f;

                            status |= vxAddArrayItems(arr, 1, &keypoint, sizeof(vx_keypoint_t));
                        }

                        num_corners++;
                    }
                }
            } // for x
        } // for y

        if (s_num_points)
            status |= vxCopyScalar(s_num_points, &num_corners, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

        status |= vxUnmapImagePatch(src, src_map_id);
    } // if ptrs non NULL

    return status;
} /* ownImageListerKernel() */

static
vx_status VX_CALLBACK own_image_lister_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (NULL != node &&
        num == dimof(image_lister_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param = 0;
        vx_image     src = 0;

        param = vxGetParameterByIndex(node, 0);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param))
        {
            status = vxQueryParameter(param, VX_PARAMETER_REF, &src, sizeof(src));

            /* validate input image */
            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)src))
            {
                vx_df_image format = 0;

                status |= vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
                if ((format == VX_DF_IMAGE_U8) ||
                    (format == VX_DF_IMAGE_S16) ||
                    (format == VX_DF_IMAGE_S32) ||
                    (format == VX_DF_IMAGE_F32))
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            /* validate output array */
            if (VX_SUCCESS == status)
            {
                vx_size capacity = 0ul; /* no defined requirement */
                vx_enum type = VX_TYPE_KEYPOINT;

                status |= vxSetMetaFormatAttribute(metas[1], VX_ARRAY_CAPACITY, &capacity, sizeof(capacity));
                status |= vxSetMetaFormatAttribute(metas[1], VX_ARRAY_ITEMTYPE, &type, sizeof(type));
            }

            /* validate output scalar (optional) */
            if (VX_SUCCESS == status && NULL != metas[2])
            {
                vx_enum type = VX_TYPE_SIZE;
                status |= vxSetMetaFormatAttribute(metas[2], VX_SCALAR_TYPE, &type, sizeof(type));
            }
        }

        if (NULL != src)
            vxReleaseImage(&src);

        if (NULL != param)
            vxReleaseParameter(&param);
    }

    return status;
} /* own_image_lister_validator() */

vx_kernel_description_t image_lister_kernel =
{
    VX_KERNEL_EXTRAS_IMAGE_LISTER,
    "org.khronos.extras.image_to_list",
    ownImageListerKernel,
    image_lister_kernel_params, dimof(image_lister_kernel_params),
    own_image_lister_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
