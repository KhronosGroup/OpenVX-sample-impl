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
 * \brief The Compare Object Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_debug_ext Debugging Extension
 */

#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>

static
vx_param_description_t compare_images_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
};

vx_status
VX_CALLBACK ownCompareImagesKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(compare_images_kernel_params))
    {
        vx_image a = (vx_image)parameters[0];
        vx_image b = (vx_image)parameters[1];
        vx_scalar diffs = (vx_scalar)parameters[2];
        vx_uint32 numDiffs = 0u;
        vx_uint32 a_width;
        vx_uint32 a_height;
        vx_size a_planes;
        vx_df_image a_format;
        vx_uint32 b_width;
        vx_uint32 b_height;
        vx_size b_planes;
        vx_df_image b_format;

        status = VX_SUCCESS;

        status |= vxQueryImage(a, VX_IMAGE_FORMAT, &a_format, sizeof(a_format));
        status |= vxQueryImage(b, VX_IMAGE_FORMAT, &b_format, sizeof(b_format));
        status |= vxQueryImage(a, VX_IMAGE_PLANES, &a_planes, sizeof(a_planes));
        status |= vxQueryImage(b, VX_IMAGE_PLANES, &b_planes, sizeof(b_planes));
        status |= vxQueryImage(a, VX_IMAGE_WIDTH, &a_width, sizeof(a_width));
        status |= vxQueryImage(b, VX_IMAGE_WIDTH, &b_width, sizeof(b_width));
        status |= vxQueryImage(a, VX_IMAGE_HEIGHT, &a_height, sizeof(a_height));
        status |= vxQueryImage(b, VX_IMAGE_HEIGHT, &b_height, sizeof(b_height));

        if (VX_SUCCESS == status &&
            (a_planes == b_planes) &&
            (a_format == b_format) &&
            (a_width  == b_width)  &&
            (a_height == b_height))
        {
            vx_int32 i;
            vx_uint32 x;
            vx_uint32 y;
            vx_uint32 p;
            void* a_base_ptrs[4] = { NULL, NULL, NULL, NULL };
            void* b_base_ptrs[4] = { NULL, NULL, NULL, NULL };
            vx_imagepatch_addressing_t a_addrs[4] = { VX_IMAGEPATCH_ADDR_INIT, VX_IMAGEPATCH_ADDR_INIT, VX_IMAGEPATCH_ADDR_INIT };
            vx_imagepatch_addressing_t b_addrs[4] = { VX_IMAGEPATCH_ADDR_INIT, VX_IMAGEPATCH_ADDR_INIT, VX_IMAGEPATCH_ADDR_INIT };
            vx_rectangle_t rect_a;
            vx_rectangle_t rect_b;
            vx_rectangle_t rect;

            status = VX_SUCCESS;

            status |= vxGetValidRegionImage(a, &rect_a);
            status |= vxGetValidRegionImage(b, &rect_b);

            if (VX_SUCCESS == status && vxFindOverlapRectangle(&rect_a, &rect_b, &rect) == vx_true_e)
            {
                vx_map_id a_map_id = 0;
                vx_map_id b_map_id = 0;

                status = VX_SUCCESS;

                for (p = 0; p < a_planes; p++)
                {
                    status |= vxMapImagePatch(a, &rect, p, &a_map_id, &a_addrs[p], &a_base_ptrs[p], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                    status |= vxMapImagePatch(b, &rect, p, &b_map_id, &b_addrs[p], &b_base_ptrs[p], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                }

                if (status != VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)node, VX_FAILURE, "Failed to get patch on a and/or b\n");
                }

                for (p = 0; p < a_planes; p++)
                {
                    for (y = 0; y < a_addrs[p].dim_y; y += a_addrs[p].step_y)
                    {
                        for (x = 0; x < a_addrs[p].dim_x; x += a_addrs[p].step_x)
                        {
                            uint8_t* a_ptr = vxFormatImagePatchAddress2d(a_base_ptrs[p], x, y, &a_addrs[p]);
                            uint8_t* b_ptr = vxFormatImagePatchAddress2d(b_base_ptrs[p], x, y, &b_addrs[p]);

                            for (i = 0; i < a_addrs[p].stride_x; i++)
                            {
                                if (a_ptr[i] != b_ptr[i])
                                {
                                    numDiffs++;
                                }
                            }
                        }
                    }
                }

                status |= vxCopyScalar(diffs, &numDiffs, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                for (p = 0; p < a_planes; p++)
                {
                    status |= vxUnmapImagePatch(a, a_map_id);
                    status |= vxUnmapImagePatch(b, b_map_id);
                }
            }

            if (numDiffs > 0)
            {
                vxAddLogEntry((vx_reference)node, VX_FAILURE, "%u differences found\n", numDiffs);
            }
        }
    } // if ptrs non NULL

    return status;
} /* ownCompareImagesKernel() */

static
vx_status VX_CALLBACK own_compare_images_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(compare_images_kernel_params) && NULL != metas)
    {
        vx_parameter param1 = vxGetParameterByIndex(node, 0); /* input image a */
        vx_parameter param2 = vxGetParameterByIndex(node, 1); /* input image b */

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2))
        {
            vx_image a = 0;
            vx_image b = 0;

            status = VX_SUCCESS;

            status |= vxQueryParameter(param1, VX_PARAMETER_REF, &a, sizeof(a));
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &b, sizeof(b));

            if (VX_SUCCESS == status)
            {
                vx_uint32 width[2];
                vx_uint32 height[2];
                vx_df_image format[2];

                status |= vxQueryImage(a, VX_IMAGE_WIDTH,  &width[0],  sizeof(width[0]));
                status |= vxQueryImage(a, VX_IMAGE_HEIGHT, &height[0], sizeof(height[0]));
                status |= vxQueryImage(a, VX_IMAGE_FORMAT, &format[0], sizeof(format[0]));

                status |= vxQueryImage(b, VX_IMAGE_WIDTH,  &width[1],  sizeof(width[1]));
                status |= vxQueryImage(b, VX_IMAGE_HEIGHT, &height[1], sizeof(height[1]));
                status |= vxQueryImage(b, VX_IMAGE_FORMAT, &format[1], sizeof(format[1]));

                if (VX_SUCCESS == status &&
                    width[0]  == width[1] &&
                    height[0] == height[1] &&
                    format[0] == format[1])
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;

                if (VX_SUCCESS == status)
                {
                    vx_enum dst_type = VX_TYPE_UINT32;
                    status |= vxSetMetaFormatAttribute(metas[2], VX_SCALAR_TYPE, &dst_type, sizeof(dst_type));
                }
            }

            if (NULL != a)
                vxReleaseImage(&a);

            if (NULL != b)
                vxReleaseImage(&b);
        }

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);
    } // if ptrs non NULL

    return VX_SUCCESS;
} /* own_compare_images_validator() */

vx_kernel_description_t compare_images_kernel =
{
    VX_KERNEL_COMPARE_IMAGE,
    "org.khronos.debug.compare_images",
    ownCompareImagesKernel,
    compare_images_kernel_params, dimof(compare_images_kernel_params),
    own_compare_images_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
