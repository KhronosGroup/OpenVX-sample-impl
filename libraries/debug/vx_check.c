
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
 * \brief The Check Object Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_debug_ext Debugging Extension
 */

#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>

typedef union _packed_value_u
{
    vx_uint8  bytes[8];
    vx_uint16  word[4];
    vx_uint32 dword[2];
    vx_uint64 qword[1];
} packed_value_u;

static
vx_param_description_t check_image_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownCheckImageKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(check_image_kernel_params))
    {
        vx_int32 i = 0;
        vx_uint32 x = 0u;
        vx_uint32 y = 0u;
        vx_uint32 p = 0u;
        vx_uint32 count = 0u;
        vx_uint32 errors = 0u;
        vx_size planes = 0u;
        vx_image image = (vx_image)parameters[0];
        vx_scalar fill = (vx_scalar)parameters[1];
        vx_scalar errs = (vx_scalar)parameters[2];
        packed_value_u value;
        vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
        vx_map_id map_id = 0;
        vx_rectangle_t rect;

        status = vxGetValidRegionImage(image, &rect);

        value.dword[0] = 0xDEADBEEF;

        status |= vxCopyScalar(fill, &value.dword[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxQueryImage(image, VX_IMAGE_PLANES, &planes, sizeof(planes));

        if (VX_SUCCESS == status)
        {
            for (p = 0u; (p < planes); p++)
            {
                void* ptr = NULL;

                status |= vxMapImagePatch(image, &rect, p, &map_id, &addr, &ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

                if ((status == VX_SUCCESS) && (ptr))
                {
                    for (y = 0; y < addr.dim_y; y += addr.step_y)
                    {
                        for (x = 0; x < addr.dim_x; x += addr.step_x)
                        {
                            vx_uint8* pixel = vxFormatImagePatchAddress2d(ptr, x, y, &addr);
                            for (i = 0; i < addr.stride_x; i++)
                            {
                                count++;
                                if (pixel[i] != value.bytes[i])
                                {
                                    errors++;
                                }
                            }
                        }
                    }

                    if (errors > 0)
                    {
                        vxAddLogEntry((vx_reference)node, VX_FAILURE,
                            "Checked %p of %u sub-pixels with 0x%08x with %u errors\n",
                            ptr, count, value.dword, errors);
                    }

                    status |= vxCopyScalar(errs, &errors, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                    status |= vxUnmapImagePatch(image, map_id);

                    if (status != VX_SUCCESS)
                    {
                        vxAddLogEntry((vx_reference)node, VX_FAILURE,
                            "Failed to set image patch for "VX_FMT_REF"\n", image);
                    }
                }
                else
                {
                    vxAddLogEntry((vx_reference)node, VX_FAILURE,
                        "Failed to get image patch for "VX_FMT_REF"\n", image);
                }
            }
        }

        if (errors > 0)
        {
            status = VX_FAILURE;
        }
    } // if ptrs non NULL

    return status;
} // ownCheckImageKernel()

static
vx_status VX_CALLBACK own_check_image_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(check_image_kernel_params) && NULL != metas)
    {
        vx_parameter param = vxGetParameterByIndex(node, 1); /* input scalar */

        if (VX_SUCCESS == vxGetStatus((vx_reference)param))
        {
            vx_scalar scalar = 0;

            status = vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));

            if (VX_SUCCESS == status)
            {
                vx_enum type = 0;

                status |= vxQueryScalar(scalar, VX_SCALAR_TYPE, &type, sizeof(type));
                if (VX_SUCCESS == status && VX_TYPE_UINT32 == type)
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_TYPE;

                if (VX_SUCCESS == status)
                {
                    vx_enum dst_type = VX_TYPE_UINT32;
                    status |= vxSetMetaFormatAttribute(metas[2], VX_SCALAR_TYPE, &dst_type, sizeof(dst_type));
                }
            }

            if (NULL != scalar)
                vxReleaseScalar(&scalar);
        }

        if (NULL != param)
            vxReleaseParameter(&param);
    } // if ptrs non NULL

    return VX_SUCCESS;
} /* own_check_image_validator() */


static
vx_param_description_t check_array_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_ARRAY,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownCheckArrayKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(check_array_kernel_params))
    {
        vx_array arr = (vx_array)parameters[0];
        vx_scalar fill = (vx_scalar)parameters[1];
        vx_scalar errs = (vx_scalar)parameters[2];
        packed_value_u value;
        vx_size num_items = 0ul;
        vx_size item_size = 0ul;
        vx_size stride = 0ul;
        vx_uint32 errors = 0;
        void* ptr = NULL;
        vx_size i = 0;
        vx_size j = 0;
        vx_map_id map_id = 0;

        value.dword[0] = 0xDEADBEEF;

        status = VX_SUCCESS;

        status |= vxCopyScalar(fill, &value.dword[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxQueryArray(arr, VX_ARRAY_NUMITEMS, &num_items, sizeof(num_items));
        status |= vxQueryArray(arr, VX_ARRAY_ITEMSIZE, &item_size, sizeof(item_size));

        status |= vxMapArrayRange(arr, 0, num_items, &map_id, &stride, &ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        if (status == VX_SUCCESS)
        {
            for (i = 0; i < num_items; ++i)
            {
                vx_uint8* item_ptr = vxFormatArrayPointer(ptr, i, stride);
                for (j = 0; j < item_size; ++j)
                {
                    if (item_ptr[j] != value.bytes[j])
                    {
                        errors++;
                    }
                }
            }

            status |= vxCopyScalar(errs, &errors, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            if (errors > 0)
            {
                vxAddLogEntry((vx_reference)node, VX_FAILURE,
                    "Check array %p of "VX_FMT_SIZE" items with 0x%02x, found %u errors\n",
                    ptr, num_items, value, errors);
            }

            status |= vxUnmapArrayRange(arr, map_id);
            if (status != VX_SUCCESS)
            {
                vxAddLogEntry((vx_reference)node, VX_FAILURE,
                    "Failed to set array range for "VX_FMT_REF"\n", arr);
            }
        }
        else
        {
            vxAddLogEntry((vx_reference)node, VX_FAILURE,
                "Failed to get array range for "VX_FMT_REF"\n", arr);
        }

        if (errors > 0)
        {
            status = VX_FAILURE;
        }
    } // if ptrs non NULL

    return status;
} // ownCheckArrayKernel()

static
vx_status VX_CALLBACK own_check_array_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(check_array_kernel_params) && NULL != metas)
    {
        vx_parameter param1 = vxGetParameterByIndex(node, 0); /* input array */
        vx_parameter param2 = vxGetParameterByIndex(node, 1); /* input scalar */

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2))
        {
            vx_array arr = 0;
            vx_scalar scalar = 0;

            status = VX_SUCCESS;

            status |= vxQueryParameter(param1, VX_PARAMETER_REF, &arr, sizeof(arr));
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &scalar, sizeof(scalar));

            if (VX_SUCCESS == status)
            {
                vx_enum item_type = 0;
                vx_enum stype = 0;

                status |= vxQueryArray(arr, VX_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
                status |= vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (VX_SUCCESS == status && stype == item_type)
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_TYPE;

                if (VX_SUCCESS == status)
                {
                    vx_enum dst_type = VX_TYPE_UINT32;
                    status |= vxSetMetaFormatAttribute(metas[2], VX_SCALAR_TYPE, &dst_type, sizeof(dst_type));
                }
            }

            if (NULL != arr)
                vxReleaseArray(&arr);

            if (NULL != scalar)
                vxReleaseScalar(&scalar);
        }

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);
    } // if ptrs non NULL

    return VX_SUCCESS;
} /* own_check_array_validator() */


vx_kernel_description_t check_image_kernel =
{
    VX_KERNEL_CHECK_IMAGE,
    "org.khronos.debug.check_image",
    ownCheckImageKernel,
    check_image_kernel_params, dimof(check_image_kernel_params),
    own_check_image_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};

vx_kernel_description_t check_array_kernel =
{
    VX_KERNEL_CHECK_ARRAY,
    "org.khronos.debug.check_array",
    ownCheckArrayKernel,
    check_array_kernel_params, dimof(check_array_kernel_params),
    own_check_array_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
