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

#include "vx_internal.h"
#include "vx_threshold.h"


static vx_bool vxIsValidThresholdType(vx_enum thresh_type)
{
    vx_bool ret = vx_false_e;
    if ((thresh_type == VX_THRESHOLD_TYPE_BINARY) ||
        (thresh_type == VX_THRESHOLD_TYPE_RANGE))
    {
        ret = vx_true_e;
    }
    return ret;
}

static vx_bool vxIsValidThresholdDataType(vx_enum data_type)
{
    vx_bool ret = vx_false_e;
    if (data_type == VX_TYPE_INT8 ||
        data_type == VX_TYPE_UINT8 ||
        data_type == VX_TYPE_INT16 ||
        data_type == VX_TYPE_UINT16 ||
        data_type == VX_TYPE_INT32 ||
        data_type == VX_TYPE_UINT32)
    {
        ret = vx_true_e;
    }
    return ret;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseThreshold(vx_threshold *t)
{
    return ownReleaseReferenceInt((vx_reference *)t, VX_TYPE_THRESHOLD, VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_threshold VX_API_CALL vxCreateThreshold(vx_context context, vx_enum thresh_type, vx_enum data_type)
{
    vx_threshold threshold = NULL;

    if (ownIsValidContext(context) == vx_true_e)
    {
        if (vxIsValidThresholdDataType(data_type) == vx_true_e)
        {
            if (vxIsValidThresholdType(thresh_type) == vx_true_e)
            {
                threshold = (vx_threshold)ownCreateReference(context, VX_TYPE_THRESHOLD, VX_EXTERNAL, &context->base);
                if (vxGetStatus((vx_reference)threshold) == VX_SUCCESS && threshold->base.type == VX_TYPE_THRESHOLD)
                {
                    threshold->thresh_type = thresh_type;
                    threshold->data_type = data_type;
                    switch(data_type)
                    {
                    case VX_TYPE_INT8:
                        threshold->true_value.U8 = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                        threshold->false_value.U8 = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                        break;
                    case VX_TYPE_UINT8:
                        threshold->true_value.U8 = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                        threshold->false_value.U8 = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                        break;
                    case VX_TYPE_UINT16:
                        threshold->true_value.U16 = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                        threshold->false_value.U16 = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                        break;
                    case VX_TYPE_INT16:
                        threshold->true_value.S16 = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                        threshold->false_value.S16 = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                        break;
                    case VX_TYPE_INT32:
                        threshold->true_value.S32 = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                        threshold->false_value.S32 = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                        break;
                    case VX_TYPE_UINT32:
                        threshold->true_value.U32 = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                        threshold->false_value.U32 = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                        break;
                    default:
                        break;
                    }
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Invalid threshold type\n");
                vxAddLogEntry(&context->base, VX_ERROR_INVALID_TYPE, "Invalid threshold type\n");
                threshold = (vx_threshold )ownGetErrorObject(context, VX_ERROR_INVALID_TYPE);
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid data type\n");
            vxAddLogEntry(&context->base, VX_ERROR_INVALID_TYPE, "Invalid data type\n");
            threshold = (vx_threshold )ownGetErrorObject(context, VX_ERROR_INVALID_TYPE);
        }
    }

    return threshold;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetThresholdAttribute(vx_threshold threshold, vx_enum attribute, const void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidSpecificReference(&threshold->base, VX_TYPE_THRESHOLD) == vx_true_e)
    {
        switch (attribute)
        {
            case VX_THRESHOLD_THRESHOLD_VALUE:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3) &&
                    (threshold->thresh_type == VX_THRESHOLD_TYPE_BINARY))
                {
                    threshold->value.S32 = *(vx_int32 *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3) &&
                        (threshold->thresh_type == VX_THRESHOLD_TYPE_BINARY))
                {
                    threshold->value = *(vx_pixel_value_t *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_THRESHOLD_LOWER:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3) &&
                    (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    threshold->lower.S32 = *(vx_int32 *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3) &&
                        (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    threshold->lower = *(vx_pixel_value_t *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_THRESHOLD_UPPER:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3) &&
                    (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    threshold->upper.S32 = *(vx_int32 *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3) &&
                        (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    threshold->upper = *(vx_pixel_value_t *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_TRUE_VALUE:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3))
                {
                    threshold->true_value.S32 = *(vx_int32 *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3))
                {
                    threshold->true_value = *(vx_pixel_value_t *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_FALSE_VALUE:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3))
                {
                    threshold->false_value.S32 = *(vx_int32 *)ptr;
                    ownWroteToReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3))
                {
                    threshold->false_value = *(vx_pixel_value_t *)ptr;
                    ownWroteToReference(&threshold->base);
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
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    VX_PRINT(VX_ZONE_API, "return %d\n", status);
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryThreshold(vx_threshold threshold, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidSpecificReference(&threshold->base, VX_TYPE_THRESHOLD) == vx_true_e)
    {
        switch (attribute)
        {
            case VX_THRESHOLD_THRESHOLD_VALUE:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3) &&
                    (threshold->thresh_type == VX_THRESHOLD_TYPE_BINARY))
                {
                    *(vx_int32 *)ptr = threshold->value.S32;
                    ownReadFromReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3) &&
                        (threshold->thresh_type == VX_THRESHOLD_TYPE_BINARY))
                {
                    *(vx_pixel_value_t *)ptr = threshold->value;
                    ownReadFromReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_THRESHOLD_LOWER:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3) &&
                    (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    *(vx_int32 *)ptr = threshold->lower.S32;
                    ownReadFromReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3) &&
                        (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    *(vx_pixel_value_t *)ptr = threshold->lower;
                    ownReadFromReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_THRESHOLD_UPPER:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3) &&
                    (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    *(vx_int32 *)ptr = threshold->upper.S32;
                    ownReadFromReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3) &&
                        (threshold->thresh_type == VX_THRESHOLD_TYPE_RANGE))
                {
                    *(vx_pixel_value_t *)ptr = threshold->upper;
                    ownReadFromReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_TRUE_VALUE:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3))
                {
                    *(vx_int32 *)ptr = threshold->true_value.S32;
                    ownReadFromReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3))
                {
                    *(vx_pixel_value_t *)ptr = threshold->true_value;
                    ownReadFromReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_FALSE_VALUE:
                if (VX_CHECK_PARAM(ptr, size, vx_int32, 0x3))
                {
                    *(vx_int32 *)ptr = threshold->false_value.S32;
                    ownReadFromReference(&threshold->base);
                }
                else if(VX_CHECK_PARAM(ptr, size, vx_pixel_value_t, 0x3))
                {
                    *(vx_pixel_value_t *)ptr = threshold->false_value;
                    ownReadFromReference(&threshold->base);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_DATA_TYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = threshold->data_type;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_TYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = threshold->thresh_type;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_INPUT_FORMAT:
                if (VX_CHECK_PARAM(ptr, size, vx_df_image, 0x3))
                {
                    *(vx_df_image *)ptr = threshold->input_format;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_THRESHOLD_OUTPUT_FORMAT:
                if (VX_CHECK_PARAM(ptr, size, vx_df_image, 0x3))
                {
                     *(vx_df_image *)ptr = threshold->output_format;
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
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    VX_PRINT(VX_ZONE_API, "return %d\n", status);
    return status;
}


static vx_bool vxIsValidThresholdFormat(vx_df_image format)
{
    vx_bool ret = vx_false_e;
    if (format == VX_DF_IMAGE_U8  ||
        format == VX_DF_IMAGE_S16 ||
        format == VX_DF_IMAGE_U16 ||
        format == VX_DF_IMAGE_S32 ||
        format == VX_DF_IMAGE_U32 )
    {
        ret = vx_true_e;
    }
    return ret;
}

static vx_bool vxIsValidThresholdFormatEx(vx_df_image format)
{
    vx_bool ret = vx_false_e;
    if (format == VX_DF_IMAGE_RGB  ||
        format == VX_DF_IMAGE_RGBX ||
        format == VX_DF_IMAGE_NV12 ||
        format == VX_DF_IMAGE_NV21 ||
        format == VX_DF_IMAGE_UYVY ||
        format == VX_DF_IMAGE_YUYV ||
        format == VX_DF_IMAGE_IYUV ||
        format == VX_DF_IMAGE_YUV4 )
    {
        ret = vx_true_e;
    }
    return ret;
}

VX_API_ENTRY vx_threshold VX_API_CALL vxCreateThresholdForImage(vx_context context,
                                                                vx_enum thresh_type,
                                                                vx_df_image input_format,
                                                                vx_df_image output_format)
{
    vx_threshold threshold = NULL;

    if (ownIsValidContext(context) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid context\n");
        threshold = NULL;
    }

    if (vxIsValidThresholdType(thresh_type) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid threshold type\n");
        vxAddLogEntry(&context->base, VX_ERROR_INVALID_TYPE, "Invalid threshold type\n");
        threshold = (vx_threshold )ownGetErrorObject(context, VX_ERROR_INVALID_TYPE);
    }

    if ( ((vxIsValidThresholdFormat  (input_format) == vx_false_e) &&
          (vxIsValidThresholdFormatEx(input_format) == vx_false_e)) ||
         ((vxIsValidThresholdFormat  (output_format) == vx_false_e) &&
          (vxIsValidThresholdFormatEx(output_format) == vx_false_e)) )
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid input or output format\n");
        vxAddLogEntry(&context->base, VX_ERROR_INVALID_TYPE, "Invalid input or output format\n");
        threshold = (vx_threshold )ownGetErrorObject(context, VX_ERROR_INVALID_TYPE);
    }


    threshold = (vx_threshold)ownCreateReference(context, VX_TYPE_THRESHOLD, VX_EXTERNAL, &context->base);
    if (vxGetStatus((vx_reference)threshold) == VX_SUCCESS && threshold->base.type == VX_TYPE_THRESHOLD)
    {
        threshold->thresh_type = thresh_type;
        threshold->input_format = input_format;
        threshold->output_format = output_format;
        switch (output_format)
        {
            case VX_DF_IMAGE_RGB:
            {
                threshold->data_type = VX_TYPE_DF_IMAGE;
                threshold->true_value.RGB[0] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->true_value.RGB[1] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->true_value.RGB[2] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->false_value.RGB[0] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                threshold->false_value.RGB[1] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                threshold->false_value.RGB[2] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                break;
            }
            case VX_DF_IMAGE_RGBX:
            {
                threshold->data_type = VX_TYPE_DF_IMAGE;
                threshold->true_value.RGBX[0] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->true_value.RGBX[1] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->true_value.RGBX[2] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->true_value.RGBX[3] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->false_value.RGBX[0] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                threshold->false_value.RGBX[1] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                threshold->false_value.RGBX[2] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                threshold->false_value.RGBX[3] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                break;
            }
            case VX_DF_IMAGE_NV12:
            case VX_DF_IMAGE_NV21:
            case VX_DF_IMAGE_UYVY:
            case VX_DF_IMAGE_YUYV:
            case VX_DF_IMAGE_IYUV:
            case VX_DF_IMAGE_YUV4:
            {
                threshold->data_type = VX_TYPE_DF_IMAGE;
                threshold->true_value.YUV[0] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->true_value.YUV[1] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->true_value.YUV[2] = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->false_value.YUV[0] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                threshold->false_value.YUV[1] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                threshold->false_value.YUV[2] = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                break;
            }
            case VX_DF_IMAGE_U8:
            {
                threshold->data_type = VX_TYPE_UINT8;
                threshold->true_value.U8  = VX_DEFAULT_THRESHOLD_TRUE_VALUE;
                threshold->false_value.U8 = VX_DEFAULT_THRESHOLD_FALSE_VALUE;
                break;
            }
            case VX_DF_IMAGE_S16:
            {
                threshold->data_type = VX_TYPE_INT16;
                threshold->true_value.S16  = VX_S16_THRESHOLD_TRUE_VALUE;
                threshold->false_value.S16 = VX_S16_THRESHOLD_FALSE_VALUE;
                break;
            }
            case VX_DF_IMAGE_U16:
            {
                threshold->data_type = VX_TYPE_UINT16;
                threshold->true_value.U16  = VX_U16_THRESHOLD_TRUE_VALUE;
                threshold->false_value.U16 = VX_U16_THRESHOLD_FALSE_VALUE;
                break;
            }
            case VX_DF_IMAGE_S32:
            {
                threshold->data_type = VX_TYPE_INT32;
                threshold->true_value.S32  = VX_S32_THRESHOLD_TRUE_VALUE;
                threshold->false_value.S32 = VX_S32_THRESHOLD_FALSE_VALUE;
                break;
            }
            case VX_DF_IMAGE_U32:
            {
                threshold->data_type = VX_TYPE_UINT32;
                threshold->true_value.U32  = VX_U32_THRESHOLD_TRUE_VALUE;
                threshold->false_value.U32 = VX_U32_THRESHOLD_FALSE_VALUE;
                break;
            }
            default:
            {
                threshold->data_type = VX_TYPE_INVALID;
                break;
            }
        }
    }
    return threshold;
}

VX_API_ENTRY vx_threshold VX_API_CALL vxCreateVirtualThresholdForImage(vx_graph graph,
                                                                       vx_enum thresh_type,
                                                                       vx_df_image input_format,
                                                                       vx_df_image output_format)
{
    vx_threshold threshold = NULL;
    vx_reference_t *gref = (vx_reference_t *)graph;
    if (ownIsValidSpecificReference(gref, VX_TYPE_GRAPH) == vx_true_e)
    {
        threshold = vxCreateThresholdForImage(gref->context, thresh_type, input_format, output_format);
        if (vxGetStatus((vx_reference)threshold) == VX_SUCCESS && threshold->base.type == VX_TYPE_THRESHOLD)
        {
            threshold->base.scope = (vx_reference_t *)graph;
            threshold->base.is_virtual = vx_true_e;
        }
        else
        {
            threshold = (vx_threshold)ownGetErrorObject(graph->base.context, VX_ERROR_INVALID_PARAMETERS);
        }
    }
    return threshold;
}

VX_API_ENTRY vx_status VX_API_CALL vxCopyThresholdOutput(vx_threshold threshold,
                                                         vx_pixel_value_t * true_value_ptr,
                                                         vx_pixel_value_t * false_value_ptr,
                                                         vx_enum usage,
                                                         vx_enum user_mem_type)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;

    if (ownIsValidSpecificReference(&threshold->base, VX_TYPE_THRESHOLD) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid reference for threshold\n");
        status = VX_ERROR_INVALID_REFERENCE;
        return status;
    }

    if (threshold->base.is_virtual == vx_true_e)
    {
        if (threshold->base.is_accessible == vx_false_e)
        {
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual threshold\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            return status;
        }
    }

    if (VX_MEMORY_TYPE_HOST == user_mem_type)
    {
        if (usage == VX_READ_ONLY)
        {
            ownSemWait(&threshold->base.lock);
            vx_size size = sizeof(vx_pixel_value_t);
            if (true_value_ptr)
            {
                memcpy(true_value_ptr, &threshold->true_value, size);
            }
            if (false_value_ptr)
            {
                memcpy(false_value_ptr, &threshold->false_value, size);
            }
            ownSemPost(&threshold->base.lock);
            ownReadFromReference(&threshold->base);
            status = VX_SUCCESS;
        }
        else if (usage == VX_WRITE_ONLY)
        {
            ownSemWait(&threshold->base.lock);
            vx_size size = sizeof(vx_pixel_value_t);
            if (true_value_ptr)
            {
                memcpy(&threshold->true_value, true_value_ptr, size);
            }
            if (false_value_ptr)
            {
                memcpy(&threshold->false_value, false_value_ptr, size);
            }
            ownSemPost(&threshold->base.lock);
            ownWroteToReference(&threshold->base);
            status = VX_SUCCESS;
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Wrong parameters for threshold\n");
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to allocate threshold\n");
        status = VX_ERROR_NO_MEMORY;
    }
    return status;
}


VX_API_ENTRY vx_status VX_API_CALL vxCopyThresholdRange(vx_threshold threshold,
                                                        vx_pixel_value_t * lower_value_ptr,
                                                        vx_pixel_value_t * upper_value_ptr,
                                                        vx_enum usage,
                                                        vx_enum user_mem_type)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;

    if (ownIsValidSpecificReference(&threshold->base, VX_TYPE_THRESHOLD) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid reference for threshold\n");
        status = VX_ERROR_INVALID_REFERENCE;
        return status;
    }

    if (threshold->base.is_virtual == vx_true_e)
    {
        if (threshold->base.is_accessible == vx_false_e)
        {
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual threshold\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            return status;
        }
    }

    if (VX_MEMORY_TYPE_HOST == user_mem_type)
    {
        if (usage == VX_READ_ONLY)
        {
            ownSemWait(&threshold->base.lock);
            vx_size size = sizeof(vx_pixel_value_t);
            if (lower_value_ptr)
            {
                memcpy(lower_value_ptr, &threshold->lower, size);
            }
            if (upper_value_ptr)
            {
                memcpy(upper_value_ptr, &threshold->upper, size);
            }
            ownSemPost(&threshold->base.lock);
            ownReadFromReference(&threshold->base);
            status = VX_SUCCESS;
        }
        else if (usage == VX_WRITE_ONLY)
        {
            ownSemWait(&threshold->base.lock);
            vx_size size = sizeof(vx_pixel_value_t);
            if (lower_value_ptr)
            {
                memcpy(&threshold->lower, lower_value_ptr, size);
            }
            if (upper_value_ptr)
            {
                memcpy(&threshold->upper, upper_value_ptr, size);
            }
            ownSemPost(&threshold->base.lock);
            ownWroteToReference(&threshold->base);
            status = VX_SUCCESS;
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Wrong parameters for threshold\n");
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to allocate threshold\n");
        status = VX_ERROR_NO_MEMORY;
    }

    return status;
}


VX_API_ENTRY vx_status VX_API_CALL vxCopyThresholdValue(vx_threshold threshold,
                                                        vx_pixel_value_t * value_ptr,
                                                        vx_enum usage,
                                                        vx_enum user_mem_type
                                                        )
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;

    if (ownIsValidSpecificReference(&threshold->base, VX_TYPE_THRESHOLD) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid reference for threshold\n");
        status = VX_ERROR_INVALID_REFERENCE;
        return status;
    }

    if (threshold->base.is_virtual == vx_true_e)
    {
        if (threshold->base.is_accessible == vx_false_e)
        {
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual threshold\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            return status;
        }
    }

    if (VX_MEMORY_TYPE_HOST == user_mem_type)
    {
        if (usage == VX_READ_ONLY)
        {
            ownSemWait(&threshold->base.lock);
            vx_size size = sizeof(vx_pixel_value_t);
            if (value_ptr)
            {
                memcpy(value_ptr, &threshold->value, size);
            }
            ownSemPost(&threshold->base.lock);
            ownReadFromReference(&threshold->base);
            status = VX_SUCCESS;
        }
        else if (usage == VX_WRITE_ONLY)
        {
            ownSemWait(&threshold->base.lock);
            vx_size size = sizeof(vx_pixel_value_t);
            if (value_ptr)
            {
                memcpy(&threshold->value, value_ptr, size);
            }
            ownSemPost(&threshold->base.lock);
            ownWroteToReference(&threshold->base);
            status = VX_SUCCESS;
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Wrong parameters for threshold\n");
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to allocate threshold\n");
        status = VX_ERROR_NO_MEMORY;
    }

    return status;
}


