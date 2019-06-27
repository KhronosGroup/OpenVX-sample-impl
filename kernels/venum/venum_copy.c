/*

 * Copyright (c) 2017-2017 The Khronos Group Inc.
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

#include <stdlib.h>
#include <string.h>
#include <venum.h>

typedef union {
    vx_char   chr;
    vx_int8   s08;
    vx_uint8  u08;
    vx_int16  s16;
    vx_uint16 u16;
    vx_int32  s32;
    vx_uint32 u32;
    vx_int64  s64;
    vx_int64  u64;
#if defined(EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT)
    vx_float16 f16;
#endif
    vx_float32 f32;
    vx_float64 f64;
    vx_df_image  fcc;
    vx_enum    enm;
    vx_size    size;
    vx_bool    boolean;
} scalar_data;

vx_status vxCopy(vx_reference input, vx_reference output)
{
    vx_status status = VX_SUCCESS;
    vx_enum user_mem_type = VX_MEMORY_TYPE_HOST;

    vx_enum type = 0;
    vxQueryReference(input, VX_REFERENCE_TYPE, &type, sizeof(type));

    switch (type)
    {
        case VX_TYPE_IMAGE:
            {
                vx_uint32 p = 0;
                vx_uint32 y = 0;
                vx_uint32 len = 0;
                vx_size planes = 0;
                void* src;
                void* dst;
                vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
                vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
                vx_map_id src_map_id = 0;
                vx_map_id dst_map_id = 0;
                vx_rectangle_t rect;
                status |= vxGetValidRegionImage((vx_image)input, &rect);
                status |= vxQueryImage((vx_image)input, VX_IMAGE_PLANES, &planes, sizeof(planes));
                for (p = 0; p < planes && status == VX_SUCCESS; p++)
                {
                    status = VX_SUCCESS;
                    src = NULL;
                    dst = NULL;
                    status |= vxMapImagePatch((vx_image)input, &rect, p, &src_map_id, &src_addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                    status |= vxMapImagePatch((vx_image)output, &rect, p, &dst_map_id, &dst_addr, &dst, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                    for (y = 0; y < src_addr.dim_y && status == VX_SUCCESS; y += src_addr.step_y)
                    {
                        vx_uint8* srcp = vxFormatImagePatchAddress2d(src, 0, y, &src_addr);
                        vx_uint8* dstp = vxFormatImagePatchAddress2d(dst, 0, y, &dst_addr);
                        len = (src_addr.stride_x * src_addr.dim_x * src_addr.scale_x) / VX_SCALE_UNITY;
                        memcpy(dstp, srcp, len);
                    }
                    if (status == VX_SUCCESS)
                    {
                        status |= vxUnmapImagePatch((vx_image)input, src_map_id);
                        status |= vxUnmapImagePatch((vx_image)output, dst_map_id);
                    }
                }
                break;
            }

        case VX_TYPE_ARRAY:
            {
                vx_size src_num_items = 0;
                vx_size dst_capacity = 0;
                vx_size src_stride = 0;
                void* srcp = NULL;
                vx_map_id map_id = 0;
                vx_array src = (vx_array)input;
                vx_array dst = (vx_array)output;
                status |= vxQueryArray(src, VX_ARRAY_NUMITEMS, &src_num_items, sizeof(src_num_items));
                status |= vxQueryArray(dst, VX_ARRAY_CAPACITY, &dst_capacity, sizeof(dst_capacity));

                if (status == VX_SUCCESS)
                {
                    status |= vxMapArrayRange(src, 0, src_num_items, &map_id, &src_stride, &srcp, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

                    if (src_num_items <= dst_capacity && status == VX_SUCCESS)
                    {
                        status |= vxTruncateArray(dst, 0);
                        status |= vxAddArrayItems(dst, src_num_items, srcp, src_stride);
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                    }

                    status |= vxUnmapArrayRange(src, map_id);
                }
                break;
            }
        case VX_TYPE_LUT:
            {
                vx_map_id map_id;
                void* lut_data = NULL;
                status = vxMapLUT((vx_lut)input, &map_id, &lut_data, VX_READ_ONLY,  user_mem_type, 0);
                if (status == VX_SUCCESS)
                    status = vxCopyLUT((vx_lut)output, lut_data, VX_WRITE_ONLY, user_mem_type);
                vxUnmapLUT((vx_lut)input, map_id);
                break;
            }
        case VX_TYPE_SCALAR:
            {
                vx_scalar scalar_input = (vx_scalar)input;
                vx_scalar scalar_output = (vx_scalar)output;
                scalar_data data;
                vx_enum input_type, output_type;
                status |= vxQueryScalar(scalar_input, VX_SCALAR_TYPE, &input_type, sizeof(input_type));
                status |= vxQueryScalar(scalar_output, VX_SCALAR_TYPE, &output_type, sizeof(output_type));
                if(status != VX_SUCCESS)
                    break;
                if(input_type != output_type)
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                    break;
                }
                vxCopyScalar(scalar_input, &data, VX_READ_ONLY, user_mem_type);
                vxCopyScalar(scalar_output, &data, VX_WRITE_ONLY, user_mem_type);
                break;
            }
        case VX_TYPE_MATRIX:
            {
                vx_matrix matrix_input = (vx_matrix)input;
                vx_matrix matrix_output = (vx_matrix)output;

                vx_enum input_type, output_type;
                vx_coordinates2d_t input_origin = {0, 0};
                vx_coordinates2d_t output_origin = {0, 0};
                vx_size input_rows = 0, input_cols = 0;
                vx_size output_rows = 0, output_cols = 0;
                vx_size input_size = 0, output_size = 0;
                status |= vxQueryMatrix(matrix_input, VX_MATRIX_TYPE, &input_type, sizeof(input_type));
                status |= vxQueryMatrix(matrix_output, VX_MATRIX_TYPE, &output_type, sizeof(output_type));
                status |= vxQueryMatrix(matrix_input, VX_MATRIX_ROWS, &input_rows, sizeof(input_rows));
                status |= vxQueryMatrix(matrix_output, VX_MATRIX_ROWS, &output_rows, sizeof(output_rows));
                status |= vxQueryMatrix(matrix_input, VX_MATRIX_COLUMNS, &input_cols, sizeof(input_cols));
                status |= vxQueryMatrix(matrix_output, VX_MATRIX_COLUMNS, &output_cols, sizeof(output_cols));
                status |= vxQueryMatrix(matrix_input, VX_MATRIX_ORIGIN, &input_origin, sizeof(input_origin));
                status |= vxQueryMatrix(matrix_output, VX_MATRIX_ORIGIN, &output_origin, sizeof(output_origin));
                status |= vxQueryMatrix(matrix_input, VX_MATRIX_SIZE, &input_size, sizeof(input_size));
                status |= vxQueryMatrix(matrix_output, VX_MATRIX_SIZE, &output_size, sizeof(output_size));
                if(status != VX_SUCCESS)
                    break;
                if(input_type != output_type ||
                        input_rows != output_rows ||
                        input_cols != output_cols ||
                        input_origin.x != output_origin.x ||
                        input_origin.y != output_origin.y ||
                        input_size != output_size)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }
                vx_size dim = 0ul;
                switch(input_type)
                {
                    case VX_TYPE_UINT8:
                        dim = sizeof(vx_uint8);
                        break;
                    case VX_TYPE_INT8:
                        dim = sizeof(vx_int8);
                        break;
                    case VX_TYPE_UINT16:
                        dim = sizeof(vx_uint16);
                        break;
                    case VX_TYPE_INT16:
                        dim = sizeof(vx_int16);
                        break;
                    case VX_TYPE_UINT32:
                        dim = sizeof(vx_uint32);
                        break;
                    case VX_TYPE_INT32:
                        dim = sizeof(vx_int32);
                        break;
                    case VX_TYPE_FLOAT32:
                        dim = sizeof(vx_float32);
                        break;
                    case VX_TYPE_UINT64:
                        dim = sizeof(vx_uint64);
                        break;
                    case VX_TYPE_INT64:
                        dim = sizeof(vx_int64);
                        break;
                    case VX_TYPE_FLOAT64:
                        dim = sizeof(vx_float64);
                        break;
                }
                if(dim == 0ul)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }
                vx_size mem_size = input_size*dim;
                void* data = malloc(mem_size);
                status |= vxCopyMatrix(matrix_input, data,  VX_READ_ONLY, user_mem_type);
                status |= vxCopyMatrix(matrix_output, data,  VX_WRITE_ONLY, user_mem_type);
                free(data);
                break;
            }
        case VX_TYPE_CONVOLUTION:
            {
                vx_convolution convolution_input = (vx_convolution)input;
                vx_convolution convolution_output = (vx_convolution)output;
                vx_size input_rows = 0, input_cols = 0;
                vx_size output_rows = 0, output_cols = 0;
                vx_size input_size = 0, output_size = 0;
                vx_uint32 input_scale = 0, output_scale = 0;
                status |= vxQueryConvolution(convolution_input, VX_CONVOLUTION_SCALE, &input_scale, sizeof(input_scale));
                status |= vxQueryConvolution(convolution_output, VX_CONVOLUTION_SCALE, &output_scale, sizeof(output_scale));
                status |= vxQueryConvolution(convolution_input, VX_CONVOLUTION_ROWS, &input_rows, sizeof(input_rows));
                status |= vxQueryConvolution(convolution_output, VX_CONVOLUTION_ROWS, &output_rows, sizeof(output_rows));
                status |= vxQueryConvolution(convolution_input, VX_CONVOLUTION_COLUMNS, &input_cols, sizeof(input_cols));
                status |= vxQueryConvolution(convolution_output, VX_CONVOLUTION_COLUMNS, &output_cols, sizeof(output_cols));
                status |= vxQueryConvolution(convolution_input, VX_CONVOLUTION_SIZE, &input_size, sizeof(input_size));
                status |= vxQueryConvolution(convolution_output, VX_CONVOLUTION_SIZE, &output_size, sizeof(output_size));
                if(status != VX_SUCCESS)
                    break;
                if(input_scale != output_scale ||
                        input_rows != output_rows ||
                        input_cols != output_cols ||
                        input_size != output_size)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }

                vx_int16 *data = (vx_int16 *)malloc(input_rows*input_cols*sizeof(vx_int16));
                status |= vxCopyConvolutionCoefficients(convolution_input, data, VX_READ_ONLY, user_mem_type);
                status |= vxCopyConvolutionCoefficients(convolution_output, data, VX_WRITE_ONLY, user_mem_type);
                free(data);
                break;
            }
        case VX_TYPE_DISTRIBUTION:
            {
                vx_distribution distribution_input = (vx_distribution)input;
                vx_distribution distribution_output = (vx_distribution)output;
                vx_size input_dims = 0, output_dims = 0;
                vx_size input_bins = 0, output_bins = 0;
                vx_size input_size = 0, output_size = 0;
                vx_uint32 input_range = 0, output_range = 0;
                vx_uint32 input_offset = 0, output_offset = 0;
                status |= vxQueryDistribution(distribution_input, VX_DISTRIBUTION_DIMENSIONS, &input_dims, sizeof(input_dims));
                status |= vxQueryDistribution(distribution_output, VX_DISTRIBUTION_DIMENSIONS, &output_dims, sizeof(output_dims));
                status |= vxQueryDistribution(distribution_input, VX_DISTRIBUTION_BINS, &input_bins, sizeof(input_bins));
                status |= vxQueryDistribution(distribution_output, VX_DISTRIBUTION_BINS, &output_bins, sizeof(output_bins));
                status |= vxQueryDistribution(distribution_input, VX_DISTRIBUTION_SIZE, &input_size, sizeof(input_size));
                status |= vxQueryDistribution(distribution_output, VX_DISTRIBUTION_SIZE, &output_size, sizeof(output_size));
                status |= vxQueryDistribution(distribution_input, VX_DISTRIBUTION_RANGE, &input_range, sizeof(input_range));
                status |= vxQueryDistribution(distribution_output, VX_DISTRIBUTION_RANGE, &output_range, sizeof(output_range));
                status |= vxQueryDistribution(distribution_input, VX_DISTRIBUTION_OFFSET, &input_offset, sizeof(input_offset));
                status |= vxQueryDistribution(distribution_output, VX_DISTRIBUTION_OFFSET, &output_offset, sizeof(output_offset));
                if(status != VX_SUCCESS)
                    break;
                if(input_range != output_range ||
                        input_offset != output_offset ||
                        input_dims != output_dims ||
                        input_bins != output_bins ||
                        input_size != output_size)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }
                vx_map_id map_id;
                void* distribution_data = NULL;
                status = vxMapDistribution(distribution_input, &map_id, &distribution_data, VX_READ_ONLY,  user_mem_type, 0);
                if (status == VX_SUCCESS)
                    status = vxCopyDistribution(distribution_output, distribution_data, VX_WRITE_ONLY, user_mem_type);
                vxUnmapDistribution(distribution_input, map_id);
                break;
            }
        case  VX_TYPE_OBJECT_ARRAY:
            {
                vx_object_array object_array_input = (vx_object_array)input;
                vx_object_array object_array_output = (vx_object_array)output;
                vx_enum input_type = VX_TYPE_INVALID, output_type= VX_TYPE_INVALID;
                vx_size input_counts = 0, output_counts = 0;
                status |= vxQueryObjectArray(object_array_input, VX_OBJECT_ARRAY_ITEMTYPE, &input_type, sizeof(input_type));
                status |= vxQueryObjectArray(object_array_output, VX_OBJECT_ARRAY_ITEMTYPE, &output_type, sizeof(output_type));
                status |= vxQueryObjectArray(object_array_input, VX_OBJECT_ARRAY_NUMITEMS, &input_counts, sizeof(input_counts));
                status |= vxQueryObjectArray(object_array_output, VX_OBJECT_ARRAY_NUMITEMS, &output_counts, sizeof(output_counts));
                if(status != VX_SUCCESS)
                    break;
                if(input_type != output_type || input_counts != output_counts)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }

                vx_reference input_item = NULL, output_item = NULL;
                for (vx_size i = 0; i < input_counts; i++)
                {
                    input_item = vxGetObjectArrayItem(object_array_input, i);
                    output_item = vxGetObjectArrayItem(object_array_output, i);

                    vxCopy(input_item,output_item);
                    vxReleaseReference(&input_item);
                    vxReleaseReference(&output_item);
                }

                break;
            }
        case  VX_TYPE_PYRAMID:
            {
                vx_pyramid pyramid_input = (vx_pyramid)input;
                vx_pyramid pyramid_output = (vx_pyramid)output;
                vx_size input_levels = 0, output_levels = 0;
                vx_int32 input_width = 0, output_width = 0;
                vx_int32 input_height = 0, output_height = 0;
                vx_float32 input_scale = 0.0, output_scale = 0.0;
                vx_df_image input_format, output_format;
                status |= vxQueryPyramid(pyramid_input, VX_PYRAMID_SCALE, &input_scale, sizeof(input_scale));
                status |= vxQueryPyramid(pyramid_output, VX_PYRAMID_SCALE, &output_scale, sizeof(output_scale));
                status |= vxQueryPyramid(pyramid_input, VX_PYRAMID_WIDTH, &input_width, sizeof(input_width));
                status |= vxQueryPyramid(pyramid_output, VX_PYRAMID_WIDTH, &output_width, sizeof(output_width));
                status |= vxQueryPyramid(pyramid_input, VX_PYRAMID_HEIGHT, &input_height, sizeof(input_height));
                status |= vxQueryPyramid(pyramid_output, VX_PYRAMID_HEIGHT, &output_height, sizeof(output_height));
                status |= vxQueryPyramid(pyramid_input, VX_PYRAMID_LEVELS, &input_levels, sizeof(input_levels));
                status |= vxQueryPyramid(pyramid_output, VX_PYRAMID_LEVELS, &output_levels, sizeof(output_levels));
                status |= vxQueryPyramid(pyramid_input, VX_PYRAMID_FORMAT, &input_format, sizeof(input_format));
                status |= vxQueryPyramid(pyramid_output, VX_PYRAMID_FORMAT, &output_format, sizeof(output_format));
                if(status != VX_SUCCESS)
                    break;
                if(input_scale != output_scale ||
                        input_width != output_width ||
                        input_height != output_height ||
                        input_format != output_format ||
                        input_levels != output_levels)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }
                vx_image input_image, output_image;
                for (vx_size level = 0; level < input_levels; level++)
                {
                    output_image = vxGetPyramidLevel(pyramid_output, level);
                    input_image = vxGetPyramidLevel(pyramid_input, level);
                    vxCopy((vx_reference)input_image, (vx_reference)output_image);
                    vxReleaseReference((vx_reference*)&input_image);
                    vxReleaseReference((vx_reference*)&output_image);
                }
                break;
            }
        case VX_TYPE_REMAP:
            {
                vx_remap remap_input = (vx_remap)input;
                vx_remap remap_output = (vx_remap)output;
                vx_uint32 input_source_width = 0, output_source_width = 0, input_source_height = 0, output_source_height = 0;
                vx_uint32 input_destination_width = 0, output_destination_width = 0, input_destination_height = 0, output_destination_height = 0;
                status |= vxQueryRemap(remap_input, VX_REMAP_SOURCE_WIDTH, &input_source_width, sizeof(input_source_width));
                status |= vxQueryRemap(remap_output, VX_REMAP_SOURCE_WIDTH, &output_source_width, sizeof(output_source_width));
                status |= vxQueryRemap(remap_input, VX_REMAP_SOURCE_HEIGHT, &input_source_height, sizeof(input_source_height));
                status |= vxQueryRemap(remap_output, VX_REMAP_SOURCE_HEIGHT, &output_source_height, sizeof(output_source_height));
                status |= vxQueryRemap(remap_input, VX_REMAP_DESTINATION_WIDTH, &input_destination_width, sizeof(input_destination_width));
                status |= vxQueryRemap(remap_output, VX_REMAP_DESTINATION_WIDTH, &output_destination_width, sizeof(output_destination_width));
                status |= vxQueryRemap(remap_input, VX_REMAP_DESTINATION_HEIGHT, &input_destination_height, sizeof(input_destination_height));
                status |= vxQueryRemap(remap_output, VX_REMAP_DESTINATION_HEIGHT, &output_destination_height, sizeof(output_destination_height));
                if(status != VX_SUCCESS)
                    break;
                if(input_source_width != output_source_width ||
                        input_source_height != output_source_height ||
                        input_destination_width != output_destination_width ||
                        input_destination_height != output_destination_height)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }

                vx_rectangle_t rect = { 0, 0, input_destination_width, input_destination_height};
                vx_size stride = input_destination_width;
                vx_size stride_y = sizeof(vx_coordinates2df_t) * (stride);
                vx_size size = stride * input_destination_height;
                vx_coordinates2df_t ptr[size];
                vxCopyRemapPatch(remap_input, &rect, stride_y, ptr, VX_TYPE_COORDINATES2DF, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyRemapPatch(remap_output, &rect, stride_y, ptr, VX_TYPE_COORDINATES2DF, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

                break;
            }
        case VX_TYPE_THRESHOLD:
            {
                vx_threshold threshold_input = (vx_threshold)input;
                vx_threshold threshold_output = (vx_threshold)output;
                vx_enum input_type = VX_TYPE_INVALID, output_type= VX_TYPE_INVALID;
                vx_df_image input_input_format, output_input_format;
                vx_df_image input_output_format, output_output_format;
                status |= vxQueryThreshold(threshold_input, VX_THRESHOLD_TYPE, &input_type, sizeof(input_type));
                status |= vxQueryThreshold(threshold_output, VX_THRESHOLD_TYPE, &output_type, sizeof(output_type));
                status |= vxQueryThreshold(threshold_input, VX_THRESHOLD_INPUT_FORMAT, &input_input_format, sizeof(input_input_format));
                status |= vxQueryThreshold(threshold_output, VX_THRESHOLD_INPUT_FORMAT, &output_input_format, sizeof(output_input_format));
                status |= vxQueryThreshold(threshold_input, VX_THRESHOLD_OUTPUT_FORMAT, &input_output_format, sizeof(input_output_format));
                status |= vxQueryThreshold(threshold_output, VX_THRESHOLD_OUTPUT_FORMAT, &output_output_format, sizeof(output_output_format));
                if(status != VX_SUCCESS)
                    break;
                if(input_type != output_type ||
                        input_input_format != output_input_format ||
                        input_output_format != output_output_format)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }
                if(input_type == VX_THRESHOLD_TYPE_BINARY)
                {
                    vx_pixel_value_t pv;
                    vxCopyThresholdValue(threshold_input, &pv, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyThresholdValue(threshold_output, &pv, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                }
                if(input_type == VX_THRESHOLD_TYPE_RANGE)
                {
                    vx_pixel_value_t pa, pb;
                    vxCopyThresholdRange(threshold_input, &pa, &pb, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    vxCopyThresholdRange(threshold_output, &pa, &pb, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                }
                vx_pixel_value_t ptrue, pfalse;
                vxCopyThresholdOutput(threshold_input, &ptrue, &pfalse, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyThresholdOutput(threshold_output, &ptrue, &pfalse, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

                break;
            }
        case  VX_TYPE_TENSOR:
            {
                vx_tensor tensor_input = (vx_tensor)input;
                vx_tensor tensor_output = (vx_tensor)output;
                vx_size input_dims_num = 0, output_dims_num = 0;
                vx_size *input_dims = NULL, *output_dims = NULL;
                vx_enum input_data_type, output_data_type;
                vx_uint8 input_fixed_point_pos = 0, output_fixed_point_pos;
                status |= vxQueryTensor(tensor_input,VX_TENSOR_NUMBER_OF_DIMS, &input_dims_num, sizeof(input_dims_num));
                status |= vxQueryTensor(tensor_output,VX_TENSOR_NUMBER_OF_DIMS, &output_dims_num, sizeof(output_dims_num));
                status |= vxQueryTensor(tensor_input, VX_TENSOR_DATA_TYPE, &input_data_type, sizeof(input_data_type));
                status |= vxQueryTensor(tensor_output, VX_TENSOR_DATA_TYPE, &output_data_type, sizeof(output_data_type));
                status |= vxQueryTensor(tensor_input, VX_TENSOR_FIXED_POINT_POSITION, &input_fixed_point_pos, sizeof(input_fixed_point_pos));
                status |= vxQueryTensor(tensor_output, VX_TENSOR_FIXED_POINT_POSITION, &output_fixed_point_pos, sizeof(output_fixed_point_pos));
                input_dims = malloc(input_dims_num * sizeof(vx_size));
                output_dims = malloc(output_dims_num * sizeof(vx_size));
                status |= vxQueryTensor(tensor_input, VX_TENSOR_DIMS, input_dims, input_dims_num * sizeof(vx_size));
                status |= vxQueryTensor(tensor_output, VX_TENSOR_DIMS, output_dims, output_dims_num * sizeof(vx_size));
                if(status != VX_SUCCESS)
                    break;
                if(input_dims_num != output_dims_num ||
                        input_data_type != output_data_type ||
                        input_fixed_point_pos != output_fixed_point_pos ||
                        memcmp(input_dims, output_dims, input_dims_num * sizeof(vx_size)) != 0)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }
                vx_size element_size = 0;
                switch(input_data_type)
                {
                    case VX_TYPE_UINT8:
                        element_size = sizeof(vx_uint8);
                        break;
                    case VX_TYPE_INT8:
                        element_size = sizeof(vx_int8);
                        break;
                    case VX_TYPE_INT16:
                        element_size = sizeof(vx_int16);
                        break;
#ifdef EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT
                    case VX_TYPE_FLOAT16:
                        element_size = sizeof(vx_float16);
                        break;
#endif
                }
                if(element_size == 0ul)
                {
                    status = VX_ERROR_INVALID_TYPE;
                    break;
                }
                vx_size * strides = malloc(sizeof(vx_size) * input_dims_num);
                vx_size * start = malloc(input_dims_num * sizeof(vx_size));
                for(vx_size i = 0; i < input_dims_num; i++)
                {
                    start[i] = 0;
                    strides[i] = i ? strides[i - 1] * input_dims[i - 1] : element_size;
                }
                const vx_size bytes = input_dims[input_dims_num - 1] * strides[input_dims_num - 1];
                void * data = malloc(bytes);
                vxCopyTensorPatch(tensor_input, input_dims_num, start, input_dims, strides, data, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyTensorPatch(tensor_output, output_dims_num, start, output_dims, strides, data, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

                free(input_dims);
                free(output_dims);
                free(strides);
                free(data);
                free(start);

                break;
            }
        default:
            {
                status = VX_ERROR_NOT_SUPPORTED;
            }
    }

    return status;
}
