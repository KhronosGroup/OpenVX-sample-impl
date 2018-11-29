/*

 * Copyright (c) 2014-2017 The Khronos Group Inc.
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

#include "vx_internal.h"
#include "vx_meta_format.h"

void ownReleaseMetaFormat(vx_meta_format *pmeta)
{
    ownReleaseReferenceInt((vx_reference *)pmeta, VX_TYPE_META_FORMAT, VX_EXTERNAL, NULL);
}

vx_meta_format ownCreateMetaFormat(vx_context context)
{
    vx_meta_format meta = NULL;

    if (ownIsValidContext(context) == vx_true_e)
    {
        meta = (vx_meta_format)ownCreateReference(context, VX_TYPE_META_FORMAT, VX_EXTERNAL, &context->base);
        if (vxGetStatus((vx_reference)meta) == VX_SUCCESS)
        {
            meta->size = sizeof(vx_meta_format_t);
            meta->type = VX_TYPE_INVALID;
        }
    }

    return meta;
}

/******************************************************************************/
// PUBLIC
/******************************************************************************/

VX_API_ENTRY vx_status VX_API_CALL vxSetMetaFormatAttribute(vx_meta_format meta, vx_enum attribute, const void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidSpecificReference(&meta->base, VX_TYPE_META_FORMAT) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (VX_TYPE(attribute) != (vx_uint32)meta->type &&
        attribute != VX_VALID_RECT_CALLBACK)
    {
        return VX_ERROR_INVALID_TYPE;
    }

    switch(attribute)
    {
        /**********************************************************************/
        case VX_IMAGE_FORMAT:
            if (VX_CHECK_PARAM(ptr, size, vx_df_image, 0x3)) {
                meta->dim.image.format = *(vx_df_image *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_IMAGE_HEIGHT:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.image.height = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_IMAGE_WIDTH:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.image.width = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_ARRAY_CAPACITY:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.array.capacity = *(vx_size *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_ARRAY_ITEMTYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3)) {
                meta->dim.array.item_type = *(vx_enum *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_PYRAMID_FORMAT:
            if (VX_CHECK_PARAM(ptr, size, vx_df_image, 0x3)) {
                meta->dim.pyramid.format = *(vx_df_image *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_PYRAMID_HEIGHT:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.pyramid.height = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_PYRAMID_WIDTH:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.pyramid.width = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_PYRAMID_LEVELS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.pyramid.levels = *(vx_size *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_PYRAMID_SCALE:
            if (VX_CHECK_PARAM(ptr, size, vx_float32, 0x3)) {
                meta->dim.pyramid.scale = *(vx_float32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_SCALAR_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3)) {
                meta->dim.scalar.type = *(vx_enum *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_MATRIX_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3)) {
                meta->dim.matrix.type = *(vx_enum *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_MATRIX_ROWS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.matrix.rows = *(vx_size *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_MATRIX_COLUMNS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.matrix.cols = *(vx_size *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_DISTRIBUTION_BINS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.distribution.bins = *(vx_size *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_DISTRIBUTION_RANGE:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.distribution.range = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_DISTRIBUTION_OFFSET:
            if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3)) {
                meta->dim.distribution.offset = *(vx_int32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_REMAP_SOURCE_WIDTH:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.remap.src_width = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REMAP_SOURCE_HEIGHT:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.remap.src_height = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REMAP_DESTINATION_WIDTH:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.remap.dst_width = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REMAP_DESTINATION_HEIGHT:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3)) {
                meta->dim.remap.dst_height = *(vx_uint32 *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_LUT_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3)) {
                meta->dim.lut.type = *(vx_enum *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_LUT_COUNT:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.lut.count = *(vx_size *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_THRESHOLD_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3)) {
                meta->dim.threshold.type = *(vx_enum *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_VALID_RECT_CALLBACK:
            if (VX_CHECK_PARAM(ptr, size, vx_kernel_image_valid_rectangle_f, 0x0)) {
                meta->set_valid_rectangle_callback = *(vx_kernel_image_valid_rectangle_f*)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_OBJECT_ARRAY_ITEMTYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3)) {
                meta->dim.object_array.item_type = *(vx_enum *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_OBJECT_ARRAY_NUMITEMS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.object_array.num_items = *(vx_enum *)ptr;
            } else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        /**********************************************************************/
        case VX_TENSOR_NUMBER_OF_DIMS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3)) {
                meta->dim.tensor.number_of_dimensions = *(vx_size *)ptr;
            }
            else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_TENSOR_DIMS:
            if (size <= (sizeof(vx_size)*VX_MAX_TENSOR_DIMENSIONS) && ((vx_size)ptr & 0x3) == 0)
            {
                memcpy(meta->dim.tensor.dimensions, ptr, size);
            }
            else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_TENSOR_DATA_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
            {
                meta->dim.tensor.data_type = *(vx_enum *)ptr;
            }
            else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_TENSOR_FIXED_POINT_POSITION:
            if (VX_CHECK_PARAM(ptr, size, vx_int8, 0x0)) //??
            {
                meta->dim.tensor.fixed_point_position = *(vx_int8 *)ptr;
            }
            else {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        default:
            status = VX_ERROR_NOT_SUPPORTED;
            break;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetMetaFormatFromReference(vx_meta_format meta, vx_reference examplar)
{
    vx_status status = VX_SUCCESS;

    if (ownIsValidSpecificReference(&meta->base, VX_TYPE_META_FORMAT) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (ownIsValidReference(examplar) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    switch (examplar->type)
    {
    case VX_TYPE_TENSOR:
    {
        vx_tensor tensor = (vx_tensor)examplar;
        meta->type = VX_TYPE_TENSOR;
        meta->dim.tensor.data_type = tensor->data_type;
        meta->dim.tensor.fixed_point_position = tensor->fixed_point_position;
        meta->dim.tensor.number_of_dimensions = tensor->number_of_dimensions;
        memcpy (meta->dim.tensor.dimensions, tensor->dimensions, sizeof(tensor->dimensions));
        break;
    }
    case VX_TYPE_IMAGE:
    {
        vx_image image = (vx_image)examplar;
        meta->type = VX_TYPE_IMAGE;
        meta->dim.image.width = image->width;
        meta->dim.image.height = image->height;
        meta->dim.image.format = image->format;
        break;
    }
    case VX_TYPE_ARRAY:
    {
        vx_array array = (vx_array)examplar;
        meta->type = VX_TYPE_ARRAY;
        meta->dim.array.item_type = array->item_type;
        meta->dim.array.capacity = array->capacity;
        break;
    }
    case VX_TYPE_PYRAMID:
    {
        vx_pyramid pyramid = (vx_pyramid)examplar;
        meta->type = VX_TYPE_PYRAMID;
        meta->dim.pyramid.width = pyramid->width;
        meta->dim.pyramid.height = pyramid->height;
        meta->dim.pyramid.format = pyramid->format;
        meta->dim.pyramid.levels = pyramid->numLevels;
        meta->dim.pyramid.scale = pyramid->scale;
        break;
    }
    case VX_TYPE_SCALAR:
    {
        vx_scalar scalar = (vx_scalar)examplar;
        meta->type = VX_TYPE_SCALAR;
        meta->dim.scalar.type = scalar->data_type;
        break;
    }
    case VX_TYPE_MATRIX:
    {
        vx_matrix matrix = (vx_matrix)examplar;
        meta->type = VX_TYPE_MATRIX;
        meta->dim.matrix.type = matrix->data_type;
        meta->dim.matrix.cols = matrix->columns;
        meta->dim.matrix.rows = matrix->rows;
        break;
    }
    case VX_TYPE_DISTRIBUTION:
    {
        vx_distribution distribution = (vx_distribution)examplar;
        meta->type = VX_TYPE_DISTRIBUTION;
        meta->dim.distribution.bins = distribution->memory.dims[0][VX_DIM_X];
        meta->dim.distribution.offset = distribution->offset_x;
        meta->dim.distribution.range = distribution->range_x;
        break;
    }
    case VX_TYPE_REMAP:
    {
        vx_remap remap = (vx_remap)examplar;
        meta->type = VX_TYPE_REMAP;
        meta->dim.remap.src_width = remap->src_width;
        meta->dim.remap.src_height = remap->src_height;
        meta->dim.remap.dst_width = remap->dst_width;
        meta->dim.remap.dst_height = remap->dst_height;
        break;
    }
    case VX_TYPE_LUT:
    {
        vx_lut_t *lut = (vx_lut_t *)examplar;
        meta->type = VX_TYPE_LUT;
        meta->dim.lut.type = lut->item_type;
        meta->dim.lut.count = lut->num_items;
        break;
    }
    case VX_TYPE_THRESHOLD:
    {
        vx_threshold threshold = (vx_threshold)examplar;
        meta->type = VX_TYPE_THRESHOLD;
        meta->dim.threshold.type = threshold->thresh_type;
        break;
    }
    case VX_TYPE_OBJECT_ARRAY:
    {
        vx_object_array objarray = (vx_object_array)examplar;
        vx_reference item = objarray->items[0];
        meta->type = VX_TYPE_OBJECT_ARRAY;
        meta->dim.object_array.item_type = objarray->item_type;
        meta->dim.object_array.num_items = objarray->num_items;

        switch (objarray->item_type)
        {
        case VX_TYPE_IMAGE:
        {
            vx_image image = (vx_image)item;
            meta->dim.image.width = image->width;
            meta->dim.image.height = image->height;
            meta->dim.image.format = image->format;
            break;
        }
        case VX_TYPE_ARRAY:
        {
            vx_array array = (vx_array)item;
            meta->dim.array.item_type = array->item_type;
            meta->dim.array.capacity = array->capacity;
            break;
        }
        case VX_TYPE_PYRAMID:
        {
            vx_pyramid pyramid = (vx_pyramid)item;
            meta->dim.pyramid.width = pyramid->width;
            meta->dim.pyramid.height = pyramid->height;
            meta->dim.pyramid.format = pyramid->format;
            meta->dim.pyramid.levels = pyramid->numLevels;
            meta->dim.pyramid.scale = pyramid->scale;
            break;
        }
        case VX_TYPE_SCALAR:
        {
            vx_scalar scalar = (vx_scalar)item;
            meta->dim.scalar.type = scalar->data_type;
            break;
        }
        case VX_TYPE_MATRIX:
        {
            vx_matrix matrix = (vx_matrix)item;
            meta->dim.matrix.type = matrix->data_type;
            meta->dim.matrix.cols = matrix->columns;
            meta->dim.matrix.rows = matrix->rows;
            break;
        }
        case VX_TYPE_DISTRIBUTION:
        {
            vx_distribution distribution = (vx_distribution)item;
            meta->dim.distribution.bins = distribution->memory.dims[0][VX_DIM_X];
            meta->dim.distribution.offset = distribution->offset_x;
            meta->dim.distribution.range = distribution->range_x;
            break;
        }
        case VX_TYPE_REMAP:
        {
            vx_remap remap = (vx_remap)item;
            meta->dim.remap.src_width = remap->src_width;
            meta->dim.remap.src_height = remap->src_height;
            meta->dim.remap.dst_width = remap->dst_width;
            meta->dim.remap.dst_height = remap->dst_height;
            break;
        }
        case VX_TYPE_LUT:
        {
            vx_lut_t *lut = (vx_lut_t *)item;
            meta->dim.lut.type = lut->item_type;
            meta->dim.lut.count = lut->num_items;
            break;
        }
        case VX_TYPE_THRESHOLD:
        {
            vx_threshold threshold = (vx_threshold)item;
            meta->dim.threshold.type = threshold->thresh_type;
            break;
        }
        default:
            status = VX_ERROR_INVALID_REFERENCE;
            break;
        }

        break;
    }
    default:
        status = VX_ERROR_INVALID_REFERENCE;
        break;
    }

    return status;
}
