/*

 * Copyright (c) 2011-2017 The Khronos Group Inc.
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
#include "vx_object_array.h"

/*==============================================================================
 PRIVATE INTERFACE
 =============================================================================*/

static vx_bool ownIsValidObjectArray(vx_object_array arr)
{
    if (arr == NULL ||
        !ownIsValidSpecificReference(&arr->base, VX_TYPE_OBJECT_ARRAY))
        return vx_false_e;

    if (arr->num_items > VX_INT_MAX_REF)
        return vx_false_e;

    for (vx_size i = 0u; i < arr->num_items; i++)
    {
        if (!ownIsValidSpecificReference(arr->items[i], arr->item_type))
            return vx_false_e;
    }

    return vx_true_e;
}

static vx_status ownInitObjectArrayInt(vx_object_array arr, vx_reference exemplar, vx_size num_items)
{
    vx_status status = VX_SUCCESS;

    vx_bool is_virtual = arr->base.is_virtual;
    vx_enum item_type = exemplar->type;

    vx_uint32 image_width, image_height;
    vx_df_image image_format;
    vx_size array_capacity;
    vx_enum array_itemtype;
    vx_uint32 pyramid_width, pyramid_height;
    vx_df_image pyramid_format;
    vx_size pyramid_levels;
    vx_float32 pyramid_scale;
    vx_enum scalar_type;
    vx_enum matrix_type;
    vx_size matrix_rows, matrix_cols;
    vx_size distribution_bins;
    vx_int32 distribution_offset;
    vx_uint32 distribution_range;
    vx_uint32 remap_srcwidth, remap_srcheight;
    vx_uint32 remap_dstwidth, remap_dstheight;
    vx_enum lut_type;
    vx_size lut_count;
    vx_enum threshold_type, threshold_data_type;
    vx_size tensor_num_dims, tensor_dims[VX_MAX_TENSOR_DIMENSIONS];
    vx_enum tensor_type;
    vx_int8 tensor_fpp;
#if defined(OPENVX_USE_USER_DATA_OBJECT)
    vx_size udo_size;
    vx_char udo_name[VX_MAX_REFERENCE_NAME];
#endif

    if (is_virtual)
    {
        vx_graph graph = (vx_graph)arr->base.scope;

        switch (item_type)
        {
            case VX_TYPE_IMAGE:
                if (vxQueryImage((vx_image)exemplar, VX_IMAGE_WIDTH, &image_width, sizeof(image_width)) != VX_SUCCESS ||
                    vxQueryImage((vx_image)exemplar, VX_IMAGE_HEIGHT, &image_height, sizeof(image_height)) != VX_SUCCESS ||
                    vxQueryImage((vx_image)exemplar, VX_IMAGE_FORMAT, &image_format, sizeof(image_format)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_ARRAY:
                if (vxQueryArray((vx_array)exemplar, VX_ARRAY_CAPACITY, &array_capacity, sizeof(array_capacity)) != VX_SUCCESS ||
                    vxQueryArray((vx_array)exemplar, VX_ARRAY_ITEMTYPE, &array_itemtype, sizeof(array_itemtype)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_PYRAMID:
                if (vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_WIDTH, &pyramid_width, sizeof(pyramid_width)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_HEIGHT, &pyramid_height, sizeof(pyramid_height)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_FORMAT, &pyramid_format, sizeof(pyramid_format)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_LEVELS, &pyramid_levels, sizeof(pyramid_levels)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_SCALE, &pyramid_scale, sizeof(pyramid_scale)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            default:
                status =  VX_ERROR_INVALID_TYPE;
                break;
        }

        if (status != VX_SUCCESS)
            return status;

        for (vx_uint32 i = 0u; i < num_items; i++)
        {
            vx_reference ref = NULL;

            switch (item_type)
            {
                case VX_TYPE_IMAGE:
                    ref = (vx_reference)vxCreateVirtualImage(graph, image_width, image_height, image_format);
                    break;
                case VX_TYPE_ARRAY:
                    ref = (vx_reference)vxCreateVirtualArray(graph, array_itemtype, array_capacity);
                    break;
                case VX_TYPE_PYRAMID:
                    ref = (vx_reference)vxCreateVirtualPyramid(graph, pyramid_levels, pyramid_scale, pyramid_width, pyramid_height, pyramid_format);
                    break;
                default:
                    ref = NULL;
                    break;
            }

            if (ownIsValidSpecificReference(ref, item_type))
            {
                arr->items[i] = ref;
                /* set the scope of the reference to the object array */
                ref->scope = (vx_reference)arr;
            }
            else
            {
                /* free all allocated references */
                for (vx_uint32 j = 0u; j < i; j++)
                {
                    ownReleaseReferenceInt(&(arr->items[j]), item_type, VX_EXTERNAL, NULL);
                }

                return VX_ERROR_NO_RESOURCES;
            }
        }
    }
    else
    {
        vx_context context = (vx_context)arr->base.scope;

        switch (item_type)
        {
            case VX_TYPE_IMAGE:
                if (vxQueryImage((vx_image)exemplar, VX_IMAGE_WIDTH, &image_width, sizeof(image_width)) != VX_SUCCESS ||
                    vxQueryImage((vx_image)exemplar, VX_IMAGE_HEIGHT, &image_height, sizeof(image_height)) != VX_SUCCESS ||
                    vxQueryImage((vx_image)exemplar, VX_IMAGE_FORMAT, &image_format, sizeof(image_format)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_ARRAY:
                if (vxQueryArray((vx_array)exemplar, VX_ARRAY_CAPACITY, &array_capacity, sizeof(array_capacity)) != VX_SUCCESS ||
                    vxQueryArray((vx_array)exemplar, VX_ARRAY_ITEMTYPE, &array_itemtype, sizeof(array_itemtype)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_PYRAMID:
                if (vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_WIDTH, &pyramid_width, sizeof(pyramid_width)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_HEIGHT, &pyramid_height, sizeof(pyramid_height)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_FORMAT, &pyramid_format, sizeof(pyramid_format)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_LEVELS, &pyramid_levels, sizeof(pyramid_levels)) != VX_SUCCESS ||
                    vxQueryPyramid((vx_pyramid)exemplar, VX_PYRAMID_SCALE, &pyramid_scale, sizeof(pyramid_scale)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_SCALAR:
                if (vxQueryScalar((vx_scalar)exemplar, VX_SCALAR_TYPE, &scalar_type, sizeof(scalar_type)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_MATRIX:
                if (vxQueryMatrix((vx_matrix)exemplar, VX_MATRIX_TYPE, &matrix_type, sizeof(matrix_type)) != VX_SUCCESS ||
                    vxQueryMatrix((vx_matrix)exemplar, VX_MATRIX_ROWS, &matrix_rows, sizeof(matrix_rows)) != VX_SUCCESS ||
                    vxQueryMatrix((vx_matrix)exemplar, VX_MATRIX_COLUMNS, &matrix_cols, sizeof(matrix_cols)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_DISTRIBUTION:
                if (vxQueryDistribution((vx_distribution)exemplar, VX_DISTRIBUTION_BINS, &distribution_bins, sizeof(distribution_bins)) != VX_SUCCESS ||
                    vxQueryDistribution((vx_distribution)exemplar, VX_DISTRIBUTION_OFFSET, &distribution_offset, sizeof(distribution_offset)) != VX_SUCCESS ||
                    vxQueryDistribution((vx_distribution)exemplar, VX_DISTRIBUTION_RANGE, &distribution_range, sizeof(distribution_range)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_REMAP:
                if (vxQueryRemap((vx_remap)exemplar, VX_REMAP_SOURCE_WIDTH, &remap_srcwidth, sizeof(remap_srcwidth)) != VX_SUCCESS ||
                    vxQueryRemap((vx_remap)exemplar, VX_REMAP_SOURCE_HEIGHT, &remap_srcheight, sizeof(remap_srcheight)) != VX_SUCCESS ||
                    vxQueryRemap((vx_remap)exemplar, VX_REMAP_DESTINATION_WIDTH, &remap_dstwidth, sizeof(remap_dstwidth)) != VX_SUCCESS ||
                    vxQueryRemap((vx_remap)exemplar, VX_REMAP_DESTINATION_HEIGHT, &remap_dstheight, sizeof(remap_dstheight)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_LUT:
                if (vxQueryLUT((vx_lut)exemplar, VX_LUT_TYPE, &lut_type, sizeof(lut_type)) != VX_SUCCESS ||
                    vxQueryLUT((vx_lut)exemplar, VX_LUT_COUNT, &lut_count, sizeof(lut_count)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_THRESHOLD:
                if (vxQueryThreshold((vx_threshold)exemplar, VX_THRESHOLD_TYPE, &threshold_type, sizeof(threshold_type)) != VX_SUCCESS ||
                    vxQueryThreshold((vx_threshold)exemplar, VX_THRESHOLD_DATA_TYPE, &threshold_data_type, sizeof(threshold_data_type)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
            case VX_TYPE_TENSOR:
                if (vxQueryTensor((vx_tensor)exemplar, VX_TENSOR_NUMBER_OF_DIMS, &tensor_num_dims, sizeof(tensor_num_dims)) != VX_SUCCESS ||
                    tensor_num_dims > VX_MAX_TENSOR_DIMENSIONS ||
                    vxQueryTensor((vx_tensor)exemplar, VX_TENSOR_DIMS, &tensor_dims[0], sizeof(vx_size) * tensor_num_dims) != VX_SUCCESS ||
                    vxQueryTensor((vx_tensor)exemplar, VX_TENSOR_DATA_TYPE, &tensor_type, sizeof(tensor_type)) != VX_SUCCESS ||
                    vxQueryTensor((vx_tensor)exemplar, VX_TENSOR_FIXED_POINT_POSITION, &tensor_fpp, sizeof(tensor_fpp)) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
#if defined(OPENVX_USE_USER_DATA_OBJECT)
            case VX_TYPE_USER_DATA_OBJECT:
                if (vxQueryUserDataObject((vx_user_data_object)exemplar, VX_USER_DATA_OBJECT_SIZE, &udo_size, sizeof(udo_size)) != VX_SUCCESS ||
                    vxQueryUserDataObject((vx_user_data_object)exemplar, VX_USER_DATA_OBJECT_NAME, &udo_name[0], sizeof(vx_char) * VX_MAX_REFERENCE_NAME) != VX_SUCCESS)
                    status = VX_ERROR_INVALID_REFERENCE;
                break;
#endif
            default:
                status =  VX_ERROR_INVALID_TYPE;
                break;
        }

        if (status != VX_SUCCESS)
            return status;

        for (vx_uint32 i = 0u; i < num_items; i++)
        {
            vx_reference ref = NULL;

            switch (item_type)
            {
                case VX_TYPE_IMAGE:
                    ref = (vx_reference)vxCreateImage(context, image_width, image_height, image_format);
                    break;
                case VX_TYPE_ARRAY:
                    ref = (vx_reference)vxCreateArray(context, array_itemtype, array_capacity);
                    break;
                case VX_TYPE_PYRAMID:
                    ref = (vx_reference)vxCreatePyramid(context, pyramid_levels, pyramid_scale, pyramid_width, pyramid_height, pyramid_format);
                    break;
                case VX_TYPE_SCALAR:
                    ref = (vx_reference)vxCreateScalar(context, scalar_type, NULL);
                    break;
                case VX_TYPE_MATRIX:
                    ref = (vx_reference)vxCreateMatrix(context, matrix_type, matrix_cols, matrix_rows);
                    break;
                case VX_TYPE_DISTRIBUTION:
                    ref = (vx_reference)vxCreateDistribution(context, distribution_bins, distribution_offset, distribution_range);
                    break;
                case VX_TYPE_REMAP:
                    ref = (vx_reference)vxCreateRemap(context, remap_srcwidth, remap_srcheight, remap_dstwidth, remap_dstheight);
                    break;
                case VX_TYPE_LUT:
                    ref = (vx_reference)vxCreateLUT(context, lut_type, lut_count);
                    break;
                case VX_TYPE_THRESHOLD:
                    ref = (vx_reference)vxCreateThreshold(context, threshold_type, threshold_data_type);
                    break;
                case VX_TYPE_TENSOR:
                    ref = (vx_reference)vxCreateTensor(context, tensor_num_dims, tensor_dims, tensor_type, tensor_fpp);
                    break;
#if defined(OPENVX_USE_USER_DATA_OBJECT)
                case VX_TYPE_USER_DATA_OBJECT:
                    ref = (vx_reference)vxCreateUserDataObject(context, udo_name, udo_size, NULL);
                    break;
#endif
                default:
                    ref = NULL;
                    break;
            }

            if (ownIsValidSpecificReference(ref, item_type))
            {
                arr->items[i] = ref;
                /* set the scope of the reference to the object array */
                ref->scope = (vx_reference)arr;
            }
            else
            {
                /* free all allocated references */
                for (vx_uint32 j = 0u; j < i; j++)
                {
                    ownReleaseReferenceInt(&(arr->items[j]), item_type, VX_EXTERNAL, NULL);
                }

                return VX_ERROR_NO_RESOURCES;
            }
        }
    }

    arr->item_type = item_type;
    arr->num_items = num_items;

    return VX_SUCCESS;
}

/*==============================================================================
 INTERNAL INTERFACE
 =============================================================================*/

vx_object_array ownCreateObjectArrayInt(vx_reference scope, vx_reference exemplar, vx_size count, vx_bool is_virtual)
{
    vx_context context = scope->context ? scope->context : (vx_context)scope;

    vx_object_array arr = (vx_object_array)ownCreateReference(context, VX_TYPE_OBJECT_ARRAY, VX_EXTERNAL, scope);
    if (vxGetStatus((vx_reference)arr) == VX_SUCCESS && arr->base.type == VX_TYPE_OBJECT_ARRAY)
    {
        arr->base.scope = scope;
        arr->base.is_virtual = is_virtual;

        if (ownInitObjectArrayInt(arr, exemplar, count) != VX_SUCCESS)
        {
            ownReleaseReferenceInt((vx_reference *)&arr, VX_TYPE_OBJECT_ARRAY, VX_EXTERNAL, NULL);
            arr = (vx_object_array)ownGetErrorObject(context, VX_ERROR_NO_MEMORY);
        }
    }
    return arr;
}

void ownDestructObjectArray(vx_reference ref)
{
    vx_object_array arr = (vx_object_array)ref;

    VX_PRINT(VX_ZONE_KERNEL, "Releasing object array "VX_FMT_REF"\n", (void *)ref);
    for (vx_uint32 i = 0u; i < arr->num_items; i++)
    {
        /* NULL means standard destructor */
        vx_status status = ownReleaseReferenceInt(&(arr->items[i]), arr->item_type, VX_EXTERNAL, NULL);

        if (status != VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid Reference!\n");
        }
    }
}

vx_bool ownValidateObjectArray(vx_object_array objarr, vx_enum item_type, vx_size num_items)
{
    vx_bool res = vx_false_e;
    if (objarr->item_type == item_type &&
        objarr->num_items == num_items)
    {
        for (vx_size i = 0u; i < objarr->num_items; i++)
        {
            if (!ownIsValidSpecificReference(objarr->items[i], item_type))
                return vx_false_e;
        }
        res = vx_true_e;
    }
    return res;
}

/*==============================================================================
 PUBLIC INTERFACE
 =============================================================================*/

VX_API_ENTRY vx_object_array VX_API_CALL vxCreateObjectArray(vx_context context, vx_reference exemplar, vx_size count)
{
    vx_object_array arr = NULL;

    if (ownIsValidContext(context) == vx_true_e)
    {
        if (ownIsValidReference(exemplar) &&
            (exemplar->type != VX_TYPE_DELAY) &&
            (exemplar->type != VX_TYPE_OBJECT_ARRAY))
        {
            arr = ownCreateObjectArrayInt((vx_reference)context, exemplar, count, vx_false_e);

            if (arr == NULL)
            {
                arr = (vx_object_array)ownGetErrorObject(context, VX_ERROR_NO_MEMORY);
            }
        }
        else
        {
            arr = (vx_object_array)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
        }
    }

    return arr;
}

VX_API_ENTRY vx_object_array VX_API_CALL vxCreateVirtualObjectArray(vx_graph graph, vx_reference exemplar, vx_size count)
{
    vx_object_array arr = NULL;

    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e)
    {
        if (ownIsValidReference(exemplar) &&
            (exemplar->type != VX_TYPE_DELAY) &&
            (exemplar->type != VX_TYPE_OBJECT_ARRAY))
        {
            arr = ownCreateObjectArrayInt((vx_reference)graph, exemplar, count, vx_true_e);

            if (arr == NULL)
            {
                arr = (vx_object_array)ownGetErrorObject(graph->base.context, VX_ERROR_NO_MEMORY);
            }
        }
        else
        {
            arr = (vx_object_array)ownGetErrorObject(graph->base.context, VX_ERROR_INVALID_PARAMETERS);
        }
    }

    return arr;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseObjectArray(vx_object_array *arr)
{
    /* NULL means standard destructor */
    return ownReleaseReferenceInt((vx_reference_t **)arr, VX_TYPE_OBJECT_ARRAY, VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryObjectArray(vx_object_array arr, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;

    if (ownIsValidObjectArray(arr) == vx_true_e)
    {
        status = VX_SUCCESS;
        switch (attribute)
        {
            case VX_OBJECT_ARRAY_ITEMTYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = arr->item_type;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;

            case VX_OBJECT_ARRAY_NUMITEMS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = arr->num_items;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;

            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }

    return status;
}

VX_API_ENTRY vx_reference VX_API_CALL vxGetObjectArrayItem(vx_object_array arr, vx_uint32 index)
{
    vx_reference item = NULL;

    if (ownIsValidObjectArray(arr) == vx_true_e)
    {
        if (index < arr->num_items)
        {
            item = arr->items[index];
            ownIncrementReference(item, VX_EXTERNAL);
        }
        else
        {
            item = (vx_reference)ownGetErrorObject(arr->base.context, VX_ERROR_INVALID_PARAMETERS);
        }
    }

    return item;
}
