/*
* Copyright (c) 2016-2017 The Khronos Group Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and/or associated documentation files (the
* "Materials"), to deal in the Materials without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Materials, and to
* permit persons to whom the Materials are furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Materials.
*
* MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
* KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
* SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
*    https://www.khronos.org/registry/
*
* THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>

static vx_status VX_CALLBACK vxHogCellsKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 6)
    {
        vx_image img = (vx_image)parameters[0];
        vx_scalar cell_width = (vx_scalar)parameters[1];
        vx_scalar cell_height = (vx_scalar)parameters[2];
        vx_scalar num_bins = (vx_scalar)parameters[3];
        vx_tensor magnitudes = (vx_tensor)parameters[4];
        vx_tensor bins = (vx_tensor)parameters[5];
        status = vxHogCells(img, cell_width, cell_height, num_bins, magnitudes, bins);
    }
    return status;
}

static vx_status VX_CALLBACK vxHogCellsInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1 || index == 2 || index == 3)
    {
        vx_scalar scalar = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum type = -1;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_INT32)
                {
                    vx_int32 para = 0;
                    if ((vxCopyScalar(scalar, &para, VX_READ_ONLY, VX_MEMORY_TYPE_HOST) == VX_SUCCESS) &&
                        (para >= 0))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxHogCellsOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_enum format;
    vx_tensor tensor;
    vx_parameter param = vxGetParameterByIndex(node, index);
    vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &tensor, sizeof(tensor));
    if (tensor && index == 4)
    {
        format = VX_TYPE_INT16;
        vx_uint8 fixed_point_pos1 = 8;
        vx_size out_num_dims;
        vx_size out_dims[2];
        status = vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
        status |= vxQueryTensor(tensor, VX_TENSOR_DIMS, out_dims, sizeof(out_dims));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos1, sizeof(fixed_point_pos1));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_DIMS, out_dims, sizeof(*out_dims) * out_num_dims);
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
    }
    else if (tensor && index == 5)
    {
        format = VX_TYPE_INT16;
        vx_uint8 fixed_point_pos1 = 8;
        vx_size out_num_dims;
        vx_size out_dims[3];
        status = vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
        status |= vxQueryTensor(tensor, VX_TENSOR_DIMS, out_dims, sizeof(out_dims));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos1, sizeof(fixed_point_pos1));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_DIMS, out_dims, sizeof(*out_dims) * out_num_dims);
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
    }
    vxReleaseTensor(&tensor);
    vxReleaseParameter(&param);
    return status;
}

static vx_param_description_t hogcells_kernel_params[] = {
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

vx_kernel_description_t hogcells_kernel = {
    VX_KERNEL_HOG_CELLS,
    "org.khronos.openvx.hog_cells",
    vxHogCellsKernel,
    hogcells_kernel_params, dimof(hogcells_kernel_params),
    NULL,
    vxHogCellsInputValidator,
    vxHogCellsOutputValidator,
    NULL,
    NULL,
};

static vx_status VX_CALLBACK vxHogFeaturesKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 6)
    {
        vx_image img = (vx_image)parameters[0];
        vx_tensor magnitudes = (vx_tensor)parameters[1];
        vx_tensor bins = (vx_tensor)parameters[2];
        vx_array hog_param = (vx_array)parameters[3];
        vx_scalar hog_param_size = (vx_scalar)parameters[4];
        vx_tensor features = (vx_tensor)parameters[5];
        status = vxHogFeatures(img, magnitudes, bins, hog_param, hog_param_size, features);
    }
    return status;
}

static vx_status VX_CALLBACK vxHogFeaturesInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_tensor mag = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vxQueryParameter(param, VX_PARAMETER_REF, &mag, sizeof(mag));
            if (mag)
            {
                vx_enum format = -1;
                vxQueryTensor(mag, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
                if (format == VX_TYPE_INT16)
                {
                    
                    status = VX_SUCCESS; 
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseTensor(&mag);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
    {
        vx_tensor mag = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vxQueryParameter(param, VX_PARAMETER_REF, &mag, sizeof(mag));
            if (mag)
            {
                vx_enum format = -1;
                vxQueryTensor(mag, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
                if (format == VX_TYPE_INT16)
                {

                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseTensor(&mag);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_array arr = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &arr, sizeof(arr));
            if (arr)
            {
                vx_enum item_type = 0;
                vxQueryArray(arr, VX_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
                if (item_type == VX_TYPE_HOG_PARAMS)
                {
                    status = VX_SUCCESS;
                }
                vxReleaseArray(&arr);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 4)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar hog_param_size = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &hog_param_size, sizeof(hog_param_size));
            if ((status == VX_SUCCESS) && (hog_param_size))
            {
                vx_enum type = 0;
                vxQueryScalar(hog_param_size, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_INT32)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&hog_param_size);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxHogFeaturesOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_enum format;
    vx_tensor tensor;
    vx_parameter param = vxGetParameterByIndex(node, index);
    vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &tensor, sizeof(tensor));
    if (tensor && index == 5)
    {
        format = VX_TYPE_INT16;
        vx_uint8 fixed_point_pos1 = 8;
        vx_size out_num_dims;
        vx_size out_dims[3];
        status = vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
        status |= vxQueryTensor(tensor, VX_TENSOR_DIMS, out_dims, sizeof(out_dims));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos1, sizeof(fixed_point_pos1));
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_DIMS, out_dims, sizeof(*out_dims) * out_num_dims);
        status |= vxSetMetaFormatAttribute(ptr, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
    }
    vxReleaseTensor(&tensor);
    vxReleaseParameter(&param);
    return status;
}

static vx_param_description_t hogfeatures_kernel_params[] = {
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

vx_kernel_description_t hogfeatures_kernel = {
    VX_KERNEL_HOG_FEATURES,
    "org.khronos.openvx.hog_features",
    vxHogFeaturesKernel,
    hogfeatures_kernel_params, dimof(hogfeatures_kernel_params),
    NULL,
    vxHogFeaturesInputValidator,
    vxHogFeaturesOutputValidator,
    NULL,
    NULL,
};
