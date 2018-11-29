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
 * \brief The Copy Object Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_debug_ext Debugging Extension
 */

#include <stdio.h>
#include <string.h>
#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>
#include "debug_k.h"

/*! \brief Declares the parameter types for \ref vxCopyImageFromPtrNode.
* \ingroup group_debug_ext
*/
static
vx_param_description_t copy_image_ptr_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_SIZE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownCopyImagePtrKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(copy_image_ptr_kernel_params))
    {
        vx_scalar input = (vx_scalar)parameters[0];
        vx_image output = (vx_image)parameters[1];
        vx_uint32 width = 0;
        vx_uint32 height = 0;
        vx_uint32 p = 0;
        vx_uint32 y = 0;
        vx_uint32 len = 0;
        vx_size planes = 0;
        void* src = NULL;
        void* dst = NULL;
        vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
        vx_map_id map_id = 0;
        vx_rectangle_t rect;
        vx_uint8* srcp = NULL;

        status = VX_SUCCESS; // assume success until an error occurs.

        status |= vxCopyScalar(input, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        srcp = (vx_uint8*)src;

        status |= vxQueryImage(output, VX_IMAGE_WIDTH,  &width,  sizeof(width));
        status |= vxQueryImage(output, VX_IMAGE_HEIGHT, &height, sizeof(height));
        status |= vxQueryImage(output, VX_IMAGE_PLANES, &planes, sizeof(planes));

        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x   = width;
        rect.end_y   = height;

        for (p = 0; p < planes && status == VX_SUCCESS; p++)
        {
            status = VX_SUCCESS;

            status |= vxMapImagePatch(output, &rect, p, &map_id, &dst_addr, &dst, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

            for (y = 0; y < height && status == VX_SUCCESS; y += dst_addr.step_y)
            {
                vx_uint8* dstp = vxFormatImagePatchAddress2d(dst, 0, y, &dst_addr);
                len = (dst_addr.stride_x * dst_addr.dim_x * dst_addr.scale_x) / VX_SCALE_UNITY;
                memcpy(dstp, srcp, len);
                srcp += len;
            }

            if (status == VX_SUCCESS)
            {
                status |= vxUnmapImagePatch(output, map_id);
            }
        }
    }
    else
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }

    return status;
} /* ownCopyImagePtrKernel() */


/*! \brief Declares the parameter types for \ref vxCopyImageNode.
* \ingroup group_debug_ext
*/
static
vx_param_description_t copy_image_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownCopyImageKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (NULL != node && NULL != parameters && num == dimof(copy_image_kernel_params))
    {
        vx_image input  = (vx_image)parameters[0];
        vx_image output = (vx_image)parameters[1];
        return ownCopyImage(input, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
} /* ownCopyImageKernel() */

static
vx_status VX_CALLBACK own_copy_image_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(copy_image_kernel_params) && NULL != metas)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0); /* input image */

        if (VX_SUCCESS == vxGetStatus((vx_reference)param))
        {
            vx_image input = 0;

            status = vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(vx_image));

            if (VX_SUCCESS == status)
            {
                vx_uint32 width = 0;
                vx_uint32 height = 0;
                vx_df_image format = VX_DF_IMAGE_VIRT;

                status |= vxQueryImage(input, VX_IMAGE_WIDTH,  &width,  sizeof(width));
                status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
                status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

                if (VX_SUCCESS == status)
                {
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_WIDTH,  &width,  sizeof(width));
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_HEIGHT, &height, sizeof(height));
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_FORMAT, &format, sizeof(format));
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            if (NULL != input)
                vxReleaseImage(&input);
        }

        if (NULL != param)
            vxReleaseParameter(&param);
    } // if ptrs non NULL

    return VX_SUCCESS;
} /* own_copy_image_validator() */


/*! \brief Declares the parameter types for \ref vxCopyArrayNode.
* \ingroup group_debug_ext
*/
static
vx_param_description_t copy_array_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownCopyArrayKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (NULL != node && NULL != parameters && num == dimof(copy_array_kernel_params))
    {
        vx_array src = (vx_array)parameters[0];
        vx_array dst = (vx_array)parameters[1];
        return ownCopyArray(src, dst);
    }
    return VX_ERROR_INVALID_PARAMETERS;
} /* ownCopyArrayKernel() */

static
vx_status VX_CALLBACK own_copy_array_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(copy_image_kernel_params) && NULL != metas)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param))
        {
            vx_array input = 0;

            status = vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(vx_array));

            if (VX_SUCCESS == status)
            {
                vx_enum item_type = VX_TYPE_INVALID;
                vx_size capacity = 0ul;

                status |= vxQueryArray(input, VX_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
                status |= vxQueryArray(input, VX_ARRAY_CAPACITY, &capacity, sizeof(capacity));

                if (VX_SUCCESS == status)
                {
                    status |= vxSetMetaFormatAttribute(metas[1], VX_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
                    status |= vxSetMetaFormatAttribute(metas[1], VX_ARRAY_CAPACITY, &capacity, sizeof(capacity));
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            if (NULL != input)
                vxReleaseArray(&input);
        }
        else
            status = VX_ERROR_INVALID_PARAMETERS;

        if (NULL != param)
            vxReleaseParameter(&param);
    } // if ptrs non NULL

    return status;
} /* own_copy_array_validator() */

static
vx_status VX_CALLBACK own_all_pass_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;
    (void)parameters;
    (void)num;
    (void)metas;

    return VX_SUCCESS;
} /* own_all_pass_validator() */


vx_kernel_description_t copy_image_ptr_kernel =
{
    VX_KERNEL_COPY_IMAGE_FROM_PTR,
    "org.khronos.debug.copy_image_ptr",
    ownCopyImagePtrKernel,
    copy_image_ptr_kernel_params, dimof(copy_image_ptr_kernel_params),
    own_all_pass_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};

vx_kernel_description_t copy_image_kernel =
{
    VX_KERNEL_DEBUG_COPY_IMAGE,
    "org.khronos.debug.copy_image",
    ownCopyImageKernel,
    copy_image_kernel_params, dimof(copy_image_kernel_params),
    own_copy_image_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};

vx_kernel_description_t copy_array_kernel =
{
    VX_KERNEL_DEBUG_COPY_ARRAY,
    "org.khronos.debug.copy_array",
    ownCopyArrayKernel,
    copy_array_kernel_params, dimof(copy_array_kernel_params),
    own_copy_array_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
