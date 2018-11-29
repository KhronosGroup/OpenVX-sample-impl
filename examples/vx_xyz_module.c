/*

 * Copyright (c) 2013-2017 The Khronos Group Inc.
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
 * \file xyz_module.c
 * \example xyz_module.c
 * \brief An example implementation of a User Extension to OpenVX.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_example_kernel Example: User Defined Kernel
 * \ingroup group_example
 * \brief An example of a user defined kernel to OpenVX and it's required parts.
 * \details Users Defined Kernels are a method by which to insert User defined
 * code to execute inside the graph concurrently with other nodes in the Graph.
 * The benefit of User Defined Kernels is that code which would otherwise have
 * to stop all other graph processing and then begin graph processing again,
 * can be run, potentially in parallel, with other nodes in the graph, with no
 * other interrupt of the graph. User Defined Kernels execute on the host
 * processor by default. OpenVX does not define a language to describe kernels,
 * but relies on the existing OS constructs of modules and symbols to be able to
 * reference local functions on the host to validate input and output data as
 * well as the invoke the kernel itself.
 */

#include <VX/vx.h>
#include <VX/vx_lib_xyz.h>
#include <stdarg.h>

/*! An internal definition of the order of the parameters to the function.
 * This list must match the parameter list in the function and in the
 * publish kernel list.
 * \ingroup group_example_kernel
 */
typedef enum _xyz_params_e {
    XYZ_PARAM_INPUT = 0,
    XYZ_PARAM_VALUE,
    XYZ_PARAM_OUTPUT,
    XYZ_PARAM_TEMP
} xyz_params_e;

/*! \brief An example parameter validator.
 * \param [in] node The handle to the node.
 * \param [in] parameters The array of parameters to be validated.
 * \param [in] num Number of parameters to be validated.
 * \param [out] metas The array of metadata used to check output parameters only.
 * \return A \ref vx_status_e enumeration.
 * \ingroup group_example_kernel
 */
vx_status VX_CALLBACK XYZValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;

    if (num != (XYZ_PARAM_TEMP + 1))
        return VX_ERROR_INVALID_PARAMETERS;

    for (vx_uint32 i = 0u; i < num; i++)
    {
        if (!parameters[i])
            return VX_ERROR_INVALID_REFERENCE;

        switch (i)
        {
            case XYZ_PARAM_INPUT:
            {
                vx_df_image df_image = 0;

                if (vxQueryImage((vx_image)parameters[i], VX_IMAGE_FORMAT, &df_image, sizeof(df_image)) != VX_SUCCESS)
                    return VX_ERROR_INVALID_PARAMETERS;

                if (df_image != VX_DF_IMAGE_U8)
                    return VX_ERROR_INVALID_VALUE;

            }   break;
            case XYZ_PARAM_VALUE:
            {
                vx_enum type = 0;
                vx_int32 value = 0;

                if (vxQueryScalar((vx_scalar)parameters[i], VX_SCALAR_TYPE, &type, sizeof(type)) != VX_SUCCESS)
                    return VX_ERROR_INVALID_PARAMETERS;

                if (type != VX_TYPE_INT32 ||
                    vxCopyScalar((vx_scalar)parameters[i], &value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST) != VX_SUCCESS)
                    return VX_ERROR_INVALID_PARAMETERS;

                if (XYZ_VALUE_MIN >= value || value >= XYZ_VALUE_MAX)
                    return VX_ERROR_INVALID_VALUE;

            }   break;
            case XYZ_PARAM_OUTPUT:
            {
                vx_image input = (vx_image)parameters[XYZ_PARAM_INPUT];
                vx_uint32 width = 0, height = 0;
                vx_df_image format = VX_DF_IMAGE_VIRT;

                if (!metas[i])
                    return VX_ERROR_INVALID_REFERENCE;

                if (vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format)) != VX_SUCCESS ||
                    vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width)) != VX_SUCCESS ||
                    vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height)) != VX_SUCCESS)
                    return VX_ERROR_INVALID_PARAMETERS;

                if (vxSetMetaFormatAttribute(metas[i], VX_IMAGE_WIDTH, &width, sizeof(width)) != VX_SUCCESS ||
                    vxSetMetaFormatAttribute(metas[i], VX_IMAGE_HEIGHT, &height, sizeof(height)) != VX_SUCCESS ||
                    vxSetMetaFormatAttribute(metas[i], VX_IMAGE_FORMAT, &format, sizeof(format)))
                    return VX_ERROR_INVALID_VALUE;

            }   break;
            case XYZ_PARAM_TEMP:
            {
                vx_size num_items = 0;

                if (vxQueryArray((vx_array)parameters[i], VX_ARRAY_NUMITEMS, &num_items, sizeof(num_items)) != VX_SUCCESS)
                    return VX_ERROR_INVALID_PARAMETERS;

                if (num_items < XYZ_TEMP_NUMITEMS)
                    return VX_ERROR_INVALID_DIMENSION;

            }   break;
            default:
                return VX_ERROR_INVALID_PARAMETERS;
        }
    }

    return VX_SUCCESS;
}

/*!
 * \brief The private kernel function for XYZ.
 * \note This is not called directly by users.
 * \param [in] node The handle to the node this kernel is instanced into.
 * \param [in] parameters The array of \ref vx_reference references.
 * \param [in] num The number of parameters in the array.
 * functions.
 * \return A \ref vx_status_e enumeration.
 * \retval VX_SUCCESS Successful return.
 * \retval VX_ERROR_INVALID_PARAMETER The input or output image were
 * of the incorrect dimensions.
 * \ingroup group_example_kernel
 */
vx_status VX_CALLBACK XYZKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 4)
    {
        vx_image input  = (vx_image)parameters[0];
        vx_scalar scalar = (vx_scalar)parameters[1];
        vx_image output = (vx_image)parameters[2];
        vx_array temp  = (vx_array)parameters[3];
        void *buf, *in = NULL, *out = NULL;
        vx_uint32 y, x;
        vx_int32 value = 0;
        vx_imagepatch_addressing_t addr1, addr2;
        vx_rectangle_t rect;
        vx_enum item_type = VX_TYPE_INVALID;
        vx_size num_items = 0, capacity = 0;
        vx_size stride = 0;
        vx_map_id map_id_input, map_id_output, map_id_array;

        status = VX_SUCCESS;

        status |= vxCopyScalar(scalar, &value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxGetValidRegionImage(input, &rect);
        status |= vxMapImagePatch(input, &rect, 0, &map_id_input, &addr1, &in, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        status |= vxMapImagePatch(output, &rect, 0, &map_id_output, &addr2, &out, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        status |= vxQueryArray(temp, VX_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
        status |= vxQueryArray(temp, VX_ARRAY_NUMITEMS, &num_items, sizeof(num_items));
        status |= vxQueryArray(temp, VX_ARRAY_CAPACITY, &capacity, sizeof(capacity));
        status |= vxMapArrayRange(temp, 0, num_items, &map_id_array, &stride, &buf, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
        for (y = 0; y < addr1.dim_y; y+=addr1.step_y)
        {
            for (x = 0; x < addr1.dim_x; x+=addr1.step_x)
            {
                // do some operation...
            }
        }
        status |= vxUnmapArrayRange(temp, map_id_array);
        status |= vxUnmapImagePatch(output, map_id_output);
        status |= vxUnmapImagePatch(input, map_id_input);
    }
    return status;
}

/*! \brief An initializer function.
 * \param [in] node The handle to the node this kernel is instanced into.
 * \param [in] parameters The array of \ref vx_reference references.
 * \param [in] num The number of parameters in the array.
 * functions.
 * \return A \ref vx_status_e enumeration.
 * \retval VX_SUCCESS Successful return.
 * \retval VX_ERROR_INVALID_PARAMETER The input or output image were
 * of the incorrect dimensions.
 * \ingroup group_example_kernel
 */
vx_status VX_CALLBACK XYZInitialize(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;
    (void)parameters;
    (void)num;

    /* XYZ requires no initialization of memory or resources */
    return VX_SUCCESS;
}

/*! \brief A deinitializer function.
 * \param [in] node The handle to the node this kernel is instanced into.
 * \param [in] parameters The array of \ref vx_reference references.
 * \param [in] num The number of parameters in the array.
 * functions.
 * \return A \ref vx_status_e enumeration.
 * \retval VX_SUCCESS Successful return.
 * \retval VX_ERROR_INVALID_PARAMETER The input or output image were
 * of the incorrect dimensions.
 * \ingroup group_example_kernel
 */
vx_status VX_CALLBACK XYZDeinitialize(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;
    (void)parameters;
    (void)num;

    /* XYZ requires no de-initialization of memory or resources */
    return VX_SUCCESS;
}

//**********************************************************************
//  PUBLIC FUNCTION
//**********************************************************************

/*! \brief The entry point into this module to add the extensions to OpenVX.
 * \param [in] context The handle to the implementation context.
 * \return A \ref vx_status_e enumeration. Returns errors if some or all kernels were not added
 * correctly.
 * \note This follows the function pointer definition of a \ref vx_publish_kernels_f
 * and uses the predefined name for the entry point, "vxPublishKernels".
 * \ingroup group_example_kernel
 */
/*VX_API_ENTRY*/ vx_status VX_API_CALL vxPublishKernels(vx_context context)
{
    vx_status status = VX_SUCCESS;
    vx_kernel kernel = vxAddUserKernel(context,
                                    "org.khronos.example.xyz",
                                    VX_KERNEL_KHR_XYZ,
                                    XYZKernel,
                                    4,
                                    XYZValidator,
                                    XYZInitialize,
                                    XYZDeinitialize);
    if (kernel)
    {
        vx_size size = XYZ_DATA_AREA;
        status = vxAddParameterToKernel(kernel, 0, VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED);
        if (status != VX_SUCCESS) goto exit;
        status = vxAddParameterToKernel(kernel, 1, VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED);
        if (status != VX_SUCCESS) goto exit;
        status = vxAddParameterToKernel(kernel, 2, VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED);
        if (status != VX_SUCCESS) goto exit;
        status = vxAddParameterToKernel(kernel, 3, VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED);
        if (status != VX_SUCCESS) goto exit;
        status = vxSetKernelAttribute(kernel, VX_KERNEL_LOCAL_DATA_SIZE, &size, sizeof(size));
        if (status != VX_SUCCESS) goto exit;
        status = vxFinalizeKernel(kernel);
        if (status != VX_SUCCESS) goto exit;
    }
exit:
    if (status != VX_SUCCESS) {
        vxRemoveKernel(kernel);
    }
    return status;
}

/*! \brief The destructor to remove a user loaded module from OpenVX.
 * \param [in] context The handle to the implementation context.
 * \return A \ref vx_status_e enumeration. Returns errors if some or all kernels were not added
 * correctly.
 * \note This follows the function pointer definition of a \ref vx_unpublish_kernels_f
 * and uses the predefined name for the entry point, "vxUnpublishKernels".
 * \ingroup group_example_kernel
 */
/*VX_API_ENTRY*/ vx_status VX_API_CALL vxUnpublishKernels(vx_context context)
{
    vx_status status = VX_SUCCESS;
    vx_kernel kernel = vxGetKernelByName(context, "org.khronos.example.xyz");
    vx_kernel kernelcpy = kernel;

    if (kernel)
    {
        status = vxReleaseKernel(&kernelcpy);
        if (status == VX_SUCCESS)
        {
            status = vxRemoveKernel(kernel);
        }
    }
    return status;
}
