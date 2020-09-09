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
#include "vx_reference.h"

#define VX_BAD_MAGIC (42)

vx_destructor_t destructors[] = {
        // frameworking
        {VX_TYPE_CONTEXT,       NULL},
        {VX_TYPE_GRAPH,         &ownDestructGraph},
        {VX_TYPE_NODE,          &ownDestructNode},
        {VX_TYPE_TARGET,        NULL},
        {VX_TYPE_KERNEL,        NULL},/*!< \todo need a destructor here */
        {VX_TYPE_PARAMETER,     &ownDestructParameter},
        {VX_TYPE_ERROR,         NULL},
        {VX_TYPE_META_FORMAT,   NULL},
#if defined(OPENVX_USE_IX) || defined(OPENVX_USE_XML)
        {VX_TYPE_IMPORT,        &ownDestructImport},
#endif
        // data objects
        {VX_TYPE_ARRAY,         &ownDestructArray},
        {VX_TYPE_OBJECT_ARRAY,  &ownDestructObjectArray},
        {VX_TYPE_CONVOLUTION,   &ownDestructConvolution},
        {VX_TYPE_DISTRIBUTION,  &ownDestructDistribution},
        {VX_TYPE_DELAY,         &ownDestructDelay},
        {VX_TYPE_IMAGE,         &ownDestructImage},
        {VX_TYPE_LUT,           &ownDestructArray},
        {VX_TYPE_MATRIX,        &ownDestructMatrix},
        {VX_TYPE_SCALAR,        &ownDestructScalar},
        {VX_TYPE_PYRAMID,       &ownDestructPyramid},
        {VX_TYPE_REMAP,         &ownDestructRemap},
        {VX_TYPE_THRESHOLD,     NULL},
        {VX_TYPE_TENSOR,        &ownDestructTensor},
};

vx_enum static_objects[] = {
        VX_TYPE_TARGET,
        VX_TYPE_KERNEL,
};

void ownInitReference(vx_reference ref, vx_context context, vx_enum type, vx_reference scope)
{
    if (ref)
    {
#if !DISABLE_ICD_COMPATIBILITY
		if(context)
			ref->platform = context->base.platform;
		else
			ref->platform = NULL;
#endif
        ref->context = context;
        ref->scope = scope;
        ref->magic = VX_MAGIC;
        ref->type = type;
        ref->internal_count = 0;
        ref->external_count = 0;
        ref->write_count = 0;
        ref->read_count = 0;
        ref->extracted = vx_false_e;
        ref->delay = NULL;
        ref->delay_slot_index = 0;
        ref->is_virtual = vx_false_e;
        ref->is_accessible = vx_false_e;
        ref->name[0] = 0;
        ownCreateSem(&ref->lock, 1);
    }
}

void ownInitReferenceForDelay(vx_reference ref, vx_delay d, vx_int32 slot_index) {
    ref->delay=d;
    ref->delay_slot_index=slot_index;
}


vx_bool ownAddReference(vx_context context, vx_reference ref)
{
    vx_uint32 r;
    vx_bool ret = vx_false_e;
    ownSemWait(&context->base.lock);
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        for (r = 0; r < VX_INT_MAX_REF; r++)
        {
            if (context->reftable[r] == NULL)
            {
                context->reftable[r] = ref;
                context->num_references++;
                ret = vx_true_e;
                break;
            }
        }
    }
    else{
        /* can't add context to itself */
        ret = vx_true_e;
    }
    ownSemPost(&context->base.lock);
    return ret;
}

vx_status ownReleaseReferenceInt(vx_reference *pref,
                        vx_enum type,
                        vx_enum reftype,
                        vx_destructor_f special_destructor)
{
    vx_status status = VX_SUCCESS;
    vx_reference ref = (pref ? *pref : NULL);
    if (ref && ownIsValidSpecificReference(ref, type) == vx_true_e)
    {
        ownPrintReference(ref);
        if (ownDecrementReference(ref, reftype) == 0)
        {
            vx_uint32 d = 0u;
            vx_destructor_f destructor = special_destructor;
            vx_enum type = ref->type;

            if (ownRemoveReference(ref->context, ref) == vx_false_e)
            {
                status = VX_ERROR_INVALID_REFERENCE;
                return status;
            }

            /* find the destructor method */
            if (!destructor)
            {
                for (d = 0u; d < dimof(destructors); d++)
                {
                    if (ref->type == destructors[d].type)
                    {
                        destructor = destructors[d].destructor;
                        break;
                    }
                }
            }

            /* if there is a destructor, call it. */
            if (destructor)
            {
                destructor(ref);
            }

            VX_PRINT(VX_ZONE_REFERENCE, ">>>> Reference count was zero, destructed object "VX_FMT_REF"\n", ref);

            ownDestroySem(&ref->lock);
            ref->magic = VX_BAD_MAGIC; /* make sure no existing copies of refs can use ref again */

            /* some objects are statically allocated. */
            for (d = 0; d < dimof(static_objects); d++) {
                if (type == static_objects[d])
                    break;
            }
            if (d == dimof(static_objects)) { // not found in list
                free(ref);
            }
        }
        *pref = NULL;
    } else {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

typedef struct _vx_type_size {
    vx_enum type;
    vx_size size;
} vx_type_size_t;

static vx_type_size_t type_sizes[] = {
    {VX_TYPE_INVALID,   0},
    // scalars
    {VX_TYPE_CHAR,      sizeof(vx_char)},
    {VX_TYPE_INT8,      sizeof(vx_int8)},
    {VX_TYPE_INT16,     sizeof(vx_int16)},
    {VX_TYPE_INT32,     sizeof(vx_int32)},
    {VX_TYPE_INT64,     sizeof(vx_int64)},
    {VX_TYPE_UINT8,     sizeof(vx_uint8)},
    {VX_TYPE_UINT16,    sizeof(vx_uint16)},
    {VX_TYPE_UINT32,    sizeof(vx_uint32)},
    {VX_TYPE_UINT64,    sizeof(vx_uint64)},
    {VX_TYPE_FLOAT32,   sizeof(vx_float32)},
    {VX_TYPE_FLOAT64,   sizeof(vx_float64)},
    {VX_TYPE_ENUM,      sizeof(vx_enum)},
    {VX_TYPE_BOOL,      sizeof(vx_bool)},
    {VX_TYPE_SIZE,      sizeof(vx_size)},
    {VX_TYPE_DF_IMAGE,    sizeof(vx_df_image)},
    // structures
    {VX_TYPE_RECTANGLE,     sizeof(vx_rectangle_t)},
    {VX_TYPE_COORDINATES2D, sizeof(vx_coordinates2d_t)},
    {VX_TYPE_COORDINATES3D, sizeof(vx_coordinates3d_t)},
    {VX_TYPE_KEYPOINT,      sizeof(vx_keypoint_t)},
    {VX_TYPE_HOUGH_LINES_PARAMS,   sizeof(vx_hough_lines_p_t)},
    {VX_TYPE_LINE_2D,   sizeof(vx_line2d_t)},
    {VX_TYPE_HOG_PARAMS,   sizeof(vx_hog_t)},
    // pseudo objects
    {VX_TYPE_ERROR,     sizeof(vx_error_t)},
    {VX_TYPE_META_FORMAT,sizeof(vx_meta_format_t)},
    {VX_TYPE_OBJECT_ARRAY, sizeof(vx_object_array_t)},
    // framework objects
    {VX_TYPE_REFERENCE, sizeof(vx_reference_t)},
    {VX_TYPE_CONTEXT,   sizeof(vx_context_t)},
    {VX_TYPE_GRAPH,     sizeof(vx_graph_t)},
    {VX_TYPE_NODE,      sizeof(vx_node_t)},
    {VX_TYPE_TARGET,    sizeof(vx_target_t)},
    {VX_TYPE_PARAMETER, sizeof(vx_parameter_t)},
    {VX_TYPE_KERNEL,    sizeof(vx_kernel_t)},
    // data objects
    {VX_TYPE_ARRAY,     sizeof(vx_array_t)},
    {VX_TYPE_CONVOLUTION, sizeof(vx_convolution_t)},
    {VX_TYPE_DELAY,     sizeof(vx_delay_t)},
    {VX_TYPE_DISTRIBUTION, sizeof(vx_distribution_t)},
    {VX_TYPE_IMAGE,     sizeof(vx_image_t)},
    {VX_TYPE_LUT,       sizeof(vx_lut_t)},
    {VX_TYPE_MATRIX,    sizeof(vx_matrix_t)},
    {VX_TYPE_PYRAMID,   sizeof(vx_pyramid_t)},
    {VX_TYPE_REMAP,     sizeof(vx_remap_t)},
    {VX_TYPE_SCALAR,    sizeof(vx_scalar_t)},
    {VX_TYPE_THRESHOLD, sizeof(vx_threshold_t)},
    {VX_TYPE_TENSOR,    sizeof(vx_tensor_t)},
#if defined(OPENVX_USE_IX) || defined(OPENVX_USE_XML)
    {VX_TYPE_IMPORT,    sizeof(vx_import_t)},
#endif
    // others
};

vx_size ownSizeOfType(vx_enum type)
{
    vx_uint32 i = 0;
    vx_size size = 0ul;
    for (i = 0; i < dimof(type_sizes); i++) {
        if (type == type_sizes[i].type) {
            size = type_sizes[i].size;
            break;
        }
    }
    return size;
}


vx_reference ownCreateReference(vx_context context, vx_enum type, vx_enum reftype, vx_reference scope)
{
    vx_size size = ownSizeOfType(type);
    vx_reference ref = (vx_reference)calloc(size, sizeof(vx_uint8));
    if (ref)
    {
        ownInitReference(ref, context, type, scope);
        ownIncrementReference(ref, reftype);
        if (ownAddReference(context, ref) == vx_false_e)
        {
            free(ref);
            VX_PRINT(VX_ZONE_ERROR, "Failed to add reference to global table!\n");
            vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES, "Failed to add to resources table\n");
            ref = (vx_reference)ownGetErrorObject(context, VX_ERROR_NO_RESOURCES);
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to allocate memory\n");
        vxAddLogEntry(&context->base, VX_ERROR_NO_MEMORY, "Failed to allocate memory\n");
        ref = (vx_reference)ownGetErrorObject(context, VX_ERROR_NO_MEMORY);
    }
    return ref;
}

vx_bool ownIsValidReference(vx_reference ref)
{
    vx_bool ret = vx_false_e;
    if (ref != NULL)
    {
        ownPrintReference(ref);
        if ( (ref->magic == VX_MAGIC) &&
             (ownIsValidType(ref->type) == vx_true_e) &&
             (( (ref->type != VX_TYPE_CONTEXT) && (ownIsValidContext(ref->context) == vx_true_e) ) ||
              ( (ref->type == VX_TYPE_CONTEXT) && (ref->context == NULL) )) )
        {
            ret = vx_true_e;
        }
        else if (ref->magic == VX_BAD_MAGIC)
        {
            VX_PRINT(VX_ZONE_ERROR, "%p has already been released and garbage collected!\n", ref);
        }
        else if (ref->type != VX_TYPE_CONTEXT)
        {
            VX_PRINT(VX_ZONE_ERROR, "%p is not a valid reference!\n", ref);
            DEBUG_BREAK();
            VX_BACKTRACE(VX_ZONE_ERROR);
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Reference was NULL\n");
    }
    return ret;
}

vx_bool ownIsValidSpecificReference(vx_reference ref, vx_enum type)
{
    vx_bool ret = vx_false_e;
    if (ref != NULL)
    {
        //ownPrintReference(ref);
        if ((ref->magic == VX_MAGIC) &&
            (ref->type == type) &&
            (ownIsValidContext(ref->context) == vx_true_e))
        {
            ret = vx_true_e;
        }
        else if (ref->type != VX_TYPE_CONTEXT)
        {
            VX_PRINT(VX_ZONE_ERROR, "%p is not a valid reference!\n", ref);
            //DEBUG_BREAK(); // this will catch any "invalid" objects, but is not useful otherwise.
            VX_BACKTRACE(VX_ZONE_WARNING);
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_WARNING, "Reference was NULL\n");
        //DEBUG_BREAK();
        VX_BACKTRACE(VX_ZONE_WARNING);
    }
    return ret;
}

vx_bool ownRemoveReference(vx_context context, vx_reference ref)
{
    vx_uint32 r;
    ownSemWait(&context->base.lock);
    for (r = 0; r < VX_INT_MAX_REF; r++)
    {
        if (context->reftable[r] == ref)
        {
            context->reftable[r] = NULL;
            context->num_references--;
            ownSemPost(&context->base.lock);
            return vx_true_e;
        }
    }
    ownSemPost(&context->base.lock);
    return vx_false_e;
}

void ownPrintReference(vx_reference ref)
{
    if (ref)
    {
        VX_PRINT(VX_ZONE_REFERENCE, "vx_reference_t:%p magic:%08x type:%08x count:[%u,%u] context:%p\n", ref, ref->magic, ref->type, ref->external_count, ref->internal_count, ref->context);
    }
}


vx_uint32 ownIncrementReference(vx_reference ref, vx_enum reftype)
{
    vx_uint32 count = 0u;
    if (ref)
    {
        ownSemWait(&ref->lock);
        if (reftype == VX_EXTERNAL || reftype == VX_BOTH)
            ref->external_count++;
        if (reftype == VX_INTERNAL || reftype == VX_BOTH)
            ref->internal_count++;
        count = ref->internal_count + ref->external_count;
        VX_PRINT(VX_ZONE_REFERENCE, "Incremented Total Reference Count to %u on "VX_FMT_REF"\n", count, ref);
        ownSemPost(&ref->lock);
    }
    return count;
}

vx_uint32 ownDecrementReference(vx_reference ref, vx_enum reftype)
{
    vx_uint32 result = UINT32_MAX;
    if (ref)
    {
        ownSemWait(&ref->lock);
        if (reftype == VX_INTERNAL || reftype == VX_BOTH) {
            if (ref->internal_count == 0) {
                VX_PRINT(VX_ZONE_WARNING, "#### INTERNAL REF COUNT IS ALREADY ZERO!!! "VX_FMT_REF" type:%08x #####\n", ref, ref->type);
                DEBUG_BREAK();
            } else {
                ref->internal_count--;
            }
        }
        if (reftype == VX_EXTERNAL || reftype == VX_BOTH) {
            if (ref->external_count == 0)
            {
                VX_PRINT(VX_ZONE_WARNING, "#### EXTERNAL REF COUNT IS ALREADY ZERO!!! "VX_FMT_REF" type:%08x #####\n", ref, ref->type);
                DEBUG_BREAK();
            }
            else
            {
                ref->external_count--;
                if ((ref->external_count == 0) && (ref->extracted == vx_true_e))
                {
                    ref->extracted = vx_false_e;
                }
            }
        }
        result = ref->internal_count + ref->external_count;
        VX_PRINT(VX_ZONE_REFERENCE, "Decremented Total Reference Count to %u on "VX_FMT_REF" type:%08x\n", result, ref, ref->type);
        ownSemPost(&ref->lock);
    }
    return result;
}

vx_uint32 ownTotalReferenceCount(vx_reference ref)
{
    vx_uint32 count = 0;
    if (ref)
    {
        ownSemWait(&ref->lock);
        count = ref->external_count + ref->internal_count;
        ownSemPost(&ref->lock);
    }
    return count;
}

void ownWroteToReference(vx_reference ref)
{
    if (ref)
    {
        ownSemWait(&ref->lock);
        ref->write_count++;
        if (ref->extracted == vx_true_e)
        {
            ownContaminateGraphs(ref);
        }
        ownSemPost(&ref->lock);
    }
}

void ownReadFromReference(vx_reference ref)
{
    if (ref)
    {
        ownSemWait(&ref->lock);
        ref->read_count++;
        ownSemPost(&ref->lock);
    }
}

/*****************************************************************************/
// PUBLIC
/*****************************************************************************/

VX_API_ENTRY vx_status VX_API_CALL vxQueryReference(vx_reference ref, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;

    /* if it is not a reference and not a context */
    if ((ownIsValidReference(ref) == vx_false_e) &&
        (ownIsValidContext((vx_context_t *)ref) == vx_false_e)) {
        return VX_ERROR_INVALID_REFERENCE;
    }
    switch (attribute)
    {
        case VX_REFERENCE_COUNT:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
            {
                *(vx_uint32 *)ptr = ref->external_count;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REFERENCE_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
            {
                *(vx_enum *)ptr = ref->type;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REFERENCE_NAME:
            if (VX_CHECK_PARAM(ptr, size, vx_char*, 0x3))
            {
                *(vx_char**)ptr = &ref->name[0];
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
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetReferenceName(vx_reference ref, const vx_char *name)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (ownIsValidReference(ref))
    {
        strncpy(ref->name, name, strnlen(name, VX_MAX_REFERENCE_NAME));
        status = VX_SUCCESS;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseReference(vx_reference* ref_ptr)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;

    vx_reference ref = (ref_ptr ? *ref_ptr : NULL);
    if (ownIsValidReference(ref) == vx_true_e)
    {
        switch (ref->type)
        {
        case VX_TYPE_CONTEXT:      status = vxReleaseContext((vx_context*)ref_ptr); break;
        case VX_TYPE_GRAPH:        status = vxReleaseGraph((vx_graph*)ref_ptr); break;
        case VX_TYPE_NODE:         status = vxReleaseNode((vx_node*)ref_ptr); break;
        case VX_TYPE_ARRAY:        status = vxReleaseArray((vx_array*)ref_ptr); break;
        case VX_TYPE_OBJECT_ARRAY: status = vxReleaseObjectArray((vx_object_array*)ref_ptr); break;
        case VX_TYPE_CONVOLUTION:  status = vxReleaseConvolution((vx_convolution*)ref_ptr); break;
        case VX_TYPE_DISTRIBUTION: status = vxReleaseDistribution((vx_distribution*)ref_ptr); break;
        case VX_TYPE_IMAGE:        status = vxReleaseImage((vx_image*)ref_ptr); break;
        case VX_TYPE_LUT:          status = vxReleaseLUT((vx_lut*)ref_ptr); break;
        case VX_TYPE_MATRIX:       status = vxReleaseMatrix((vx_matrix*)ref_ptr); break;
        case VX_TYPE_PYRAMID:      status = vxReleasePyramid((vx_pyramid*)ref_ptr); break;
        case VX_TYPE_REMAP:        status = vxReleaseRemap((vx_remap*)ref_ptr); break;
        case VX_TYPE_SCALAR:       status = vxReleaseScalar((vx_scalar*)ref_ptr); break;
        case VX_TYPE_THRESHOLD:    status = vxReleaseThreshold((vx_threshold*)ref_ptr); break;
        case VX_TYPE_DELAY:        status = vxReleaseDelay((vx_delay*)ref_ptr); break;
        case VX_TYPE_KERNEL:       status = vxReleaseKernel((vx_kernel*)ref_ptr); break;
        case VX_TYPE_PARAMETER:    status = vxReleaseParameter((vx_parameter*)ref_ptr); break;
        case VX_TYPE_TENSOR:       status = vxReleaseTensor((vx_tensor*)ref_ptr); break;
#if defined(OPENVX_USE_IX) || defined(OPENVX_USE_XML)
        case VX_TYPE_IMPORT:       status = vxReleaseImport((vx_import*)ref_ptr); break;
#endif
        default:
            break;
        }
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxRetainReference(vx_reference ref)
{
    vx_status status = VX_SUCCESS;

    if (ownIsValidReference(ref) == vx_true_e)
    {
        ownIncrementReference(ref, VX_EXTERNAL);
    }
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }

    return status;
}
