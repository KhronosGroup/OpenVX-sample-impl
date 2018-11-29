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
#include "vx_delay.h"

#define vxIsValidDelay(d) ((d) && ownIsValidSpecificReference((vx_reference)(d), VX_TYPE_DELAY))
#define vxIsValidGraph(g) ((g) && ownIsValidSpecificReference((vx_reference)(g), VX_TYPE_GRAPH))

vx_bool ownAddAssociationToDelay(vx_reference value, vx_node n, vx_uint32 i)
{

    vx_delay delay = value->delay;
    vx_int32 delay_index = value->delay_slot_index;

    vx_int32 index = (delay->index + delay->count - abs(delay_index)) % (vx_int32)delay->count;

    if (delay->set[index].node == 0) // head is empty
    {
        delay->set[index].node = n;
        delay->set[index].index = i;
    }
    else
    {
        vx_delay_param_t **ptr = &delay->set[index].next;
        do {
            if (*ptr == NULL)
            {
                *ptr = VX_CALLOC(vx_delay_param_t);
                if (*ptr != NULL)
                {
                    (*ptr)->node = n;
                    (*ptr)->index = i;
                    break;
                }
                else {
                    return vx_false_e;
                }
            }
            else {
                ptr = &((*ptr)->next);
            }
        } while (1);
    }

    // Increment a reference to the delay
    ownIncrementReference((vx_reference)delay, VX_INTERNAL);

    return vx_true_e;
}

vx_bool ownRemoveAssociationToDelay(vx_reference value, vx_node n, vx_uint32 i)
{
    vx_delay delay = value->delay;
    vx_int32 delay_index = value->delay_slot_index;

    vx_int32 index = (delay->index + delay->count - abs(delay_index)) % (vx_int32)delay->count;

    if (index >= (vx_int32)delay->count) {
        return vx_false_e;
    }

    if (delay->set[index].node == n && delay->set[index].index == i) // head is a match
    {
        delay->set[index].node = 0;
        delay->set[index].index = 0;
    }
    else
    {
        vx_delay_param_t **ptr = &delay->set[index].next;
        vx_delay_param_t *next = NULL;
        do {
            if (*ptr != NULL)
            {
                if ((*ptr)->node == n && (*ptr)->index == i)
                {
                    (*ptr)->node = 0;
                    (*ptr)->index = 0;
                    next = (*ptr)->next;
                    free(*ptr);
                    *ptr = next;
                    break;
                }
                else {
                   ptr = &((*ptr)->next);
                }
            }
            else {
                return vx_false_e;
            }
        } while (1);
    }

    // Release the delay
    {
        vx_reference ref=(vx_reference)delay;
        ownReleaseReferenceInt(&ref, VX_TYPE_DELAY, VX_INTERNAL, NULL);
    }
    return vx_true_e;
}




/******************************************************************************/
/* PUBLIC INTERFACE */
/******************************************************************************/

VX_API_ENTRY vx_reference VX_API_CALL vxGetReferenceFromDelay(vx_delay delay, vx_int32 index)
{
    vx_reference ref = 0;
    if (vxIsValidDelay(delay) == vx_true_e)
    {
        if ((vx_uint32)abs(index) < delay->count)
        {
            vx_int32 i = (delay->index + abs(index)) % (vx_int32)delay->count;
            ref = delay->refs[i];
            VX_PRINT(VX_ZONE_DELAY, "Retrieving relative index %d => " VX_FMT_REF  " from Delay (%d)\n", index, ref, i);
        }
        else
        {
            vxAddLogEntry(&delay->base, VX_ERROR_INVALID_PARAMETERS, "Failed to retrieve reference from delay by index %d\n", index);
            ref = (vx_reference_t *)ownGetErrorObject(delay->base.context, VX_ERROR_INVALID_PARAMETERS);
        }
    }
    return ref;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryDelay(vx_delay delay, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (vxIsValidDelay(delay) == vx_true_e)
    {
        switch (attribute)
        {
            case VX_DELAY_TYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                    *(vx_enum *)ptr = delay->type;
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
                break;
            case VX_DELAY_SLOTS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                    *(vx_size *)ptr = (vx_size)delay->count;
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
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
    VX_PRINT(VX_ZONE_API, "%s returned %d\n",__FUNCTION__, status);
    return status;
}

void ownDestructDelay(vx_reference ref) {
    vx_uint32 i = 0;
    vx_delay delay = (vx_delay)ref;

    if (vxIsValidDelay(delay) && (delay->type == VX_TYPE_PYRAMID) && (delay->pyr != NULL))
    {
        vx_uint32 numLevels = (vx_uint32)(((vx_pyramid)delay->refs[0])->numLevels);
        /* release pyramid delays */
        for (i = 0; i < numLevels; ++i)
        {
            ownReleaseReferenceInt((vx_reference *)&(delay->pyr[i]), VX_TYPE_DELAY, VX_INTERNAL, &ownDestructDelay);
        }
        free(delay->pyr);
    }

    for (i = 0; i < delay->count; i++)
    {
        ownReleaseReferenceInt(&delay->refs[i], delay->type, VX_INTERNAL, NULL);
    }

    if (delay->set)
    {
        for (i = 0; i < delay->count; i++)
        {
            vx_delay_param_t *cur = delay->set[i].next;
            while (cur != NULL)
            {
                vx_delay_param_t *next = cur->next;
                free(cur);
                cur = next;
            }
        }
        free(delay->set);
    }

    if (delay->refs) {
        free(delay->refs);
    }
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseDelay(vx_delay *d)
{
    return ownReleaseReferenceInt((vx_reference *)d, VX_TYPE_DELAY, VX_EXTERNAL, &ownDestructDelay);
}

VX_API_ENTRY vx_delay VX_API_CALL vxCreateDelay(vx_context context,
                              vx_reference exemplar,
                              vx_size count)
{
    vx_delay delay = NULL;
    vx_enum invalid_types[] = {
        VX_TYPE_CONTEXT,
        VX_TYPE_GRAPH,
        VX_TYPE_NODE,
        VX_TYPE_KERNEL,
        VX_TYPE_TARGET,
        VX_TYPE_PARAMETER,
        VX_TYPE_REFERENCE,
        VX_TYPE_DELAY, // no delays of delays
#ifdef OPENVX_KHR_XML
        VX_TYPE_IMPORT,
#endif
    };
    vx_uint32 t = 0u;

    if (ownIsValidContext(context) == vx_false_e)
        return 0;

    if (ownIsValidReference(exemplar) == vx_false_e)
        return (vx_delay)ownGetErrorObject(context, VX_ERROR_INVALID_REFERENCE);

    for (t = 0u; t < dimof(invalid_types); t++)
    {
        if (exemplar->type == invalid_types[t])
        {
            VX_PRINT(VX_ZONE_ERROR, "Attempted to create delay of invalid object type!\n");
            vxAddLogEntry((vx_reference)context, VX_ERROR_INVALID_REFERENCE, "Attempted to create delay of invalid object type!\n");
            return (vx_delay)ownGetErrorObject(context, VX_ERROR_INVALID_REFERENCE);
        }
    }

    delay = (vx_delay)ownCreateReference(context, VX_TYPE_DELAY, VX_EXTERNAL, &context->base);
    if (vxGetStatus((vx_reference)delay) == VX_SUCCESS && delay->base.type == VX_TYPE_DELAY)
    {
        vx_size i = 0;
        delay->pyr = NULL;
        delay->set = (vx_delay_param_t *)calloc(count, sizeof(vx_delay_param_t));
        delay->refs = (vx_reference *)calloc(count, sizeof(vx_reference));
        delay->type = exemplar->type;
        delay->count = count;
        VX_PRINT(VX_ZONE_DELAY, "Creating Delay of %u objects of type %x!\n", count, exemplar->type);
        for (i = 0; i < count; i++)
        {
            switch (exemplar->type)
            {
                case VX_TYPE_IMAGE:
                {
                    vx_image image = (vx_image )exemplar;
                    delay->refs[i] = (vx_reference)vxCreateImage(context, image->width, image->height, image->format);
                    break;
                }
                case VX_TYPE_ARRAY:
                {
                    vx_array arr = (vx_array )exemplar;
                    delay->refs[i] = (vx_reference)vxCreateArray(context, arr->item_type, arr->capacity);
                    break;
                }
                case VX_TYPE_MATRIX:
                {
                    vx_matrix mat = (vx_matrix)exemplar;
                    delay->refs[i] = (vx_reference)vxCreateMatrix(context, mat->data_type, mat->columns, mat->rows);
                    break;
                }
                case VX_TYPE_CONVOLUTION:
                {
                    vx_convolution conv = (vx_convolution)exemplar;
                    vx_convolution conv2 = vxCreateConvolution(context, conv->base.columns, conv->base.rows);
                    conv2->scale = conv->scale;
                    delay->refs[i] = (vx_reference)conv2;
                    break;
                }
                case VX_TYPE_DISTRIBUTION:
                {
                    vx_distribution dist = (vx_distribution)exemplar;
                    delay->refs[i] = (vx_reference)vxCreateDistribution(context, dist->memory.dims[0][VX_DIM_X], dist->offset_x, dist->range_x);
                    break;
                }
                case VX_TYPE_REMAP:
                {
                    vx_remap remap = (vx_remap)exemplar;
                    delay->refs[i] = (vx_reference)vxCreateRemap(context, remap->src_width, remap->src_height, remap->dst_width, remap->dst_height);
                    break;
                }
                case VX_TYPE_LUT:
                {
                    vx_lut_t *lut = (vx_lut_t *)exemplar;
                    delay->refs[i] = (vx_reference)vxCreateLUT(context, lut->item_type, lut->capacity);
                    break;
                }
                case VX_TYPE_PYRAMID:
                {
                    vx_pyramid pyramid = (vx_pyramid )exemplar;
                    delay->refs[i] = (vx_reference)vxCreatePyramid(context, pyramid->numLevels, pyramid->scale, pyramid->width, pyramid->height, pyramid->format);
                    break;
                }
                case VX_TYPE_THRESHOLD:
                {
                    vx_threshold thresh = (vx_threshold )exemplar;
                    delay->refs[i] = (vx_reference)vxCreateThreshold(context, thresh->thresh_type, VX_TYPE_UINT8);
                    break;
                }
                case VX_TYPE_SCALAR:
                {
                    vx_scalar scalar = (vx_scalar )exemplar;
                    delay->refs[i] = (vx_reference)vxCreateScalar(context, scalar->data_type, NULL);
                    break;
                }
                default:
                    break;
            }
            /* set the object as a delay element */
            ownInitReferenceForDelay(delay->refs[i], delay, (vx_int32)i);
            /* change the counting from external to internal */
            ownIncrementReference(delay->refs[i], VX_INTERNAL);
            ownDecrementReference(delay->refs[i], VX_EXTERNAL);
            /* set the scope to the delay */
            ((vx_reference )delay->refs[i])->scope = (vx_reference )delay;
        }

        if (exemplar->type == VX_TYPE_PYRAMID)
        {
            /* create internal delays for each pyramid level */
            vx_size j = 0;
            vx_size numLevels = ((vx_pyramid)exemplar)->numLevels;
            delay->pyr = (vx_delay *)calloc(numLevels, sizeof(vx_delay));
            vx_delay pyrdelay = NULL;
            for (j = 0; j < numLevels; ++j)
            {
                pyrdelay = (vx_delay)ownCreateReference(context, VX_TYPE_DELAY, VX_INTERNAL, (vx_reference)delay);
                delay->pyr[j] = pyrdelay;
                if (vxGetStatus((vx_reference)pyrdelay) == VX_SUCCESS && pyrdelay->base.type == VX_TYPE_DELAY)
                {
                    pyrdelay->set = (vx_delay_param_t *)calloc(count, sizeof(vx_delay_param_t));
                    pyrdelay->refs = (vx_reference *)calloc(count, sizeof(vx_reference));
                    pyrdelay->type = VX_TYPE_IMAGE;
                    pyrdelay->count = count;
                    for (i = 0; i < count; i++)
                    {
                        pyrdelay->refs[i] = (vx_reference)vxGetPyramidLevel((vx_pyramid)delay->refs[i], (vx_uint32)j);
                        /* set the object as a delay element */
                        ownInitReferenceForDelay(pyrdelay->refs[i], pyrdelay, (vx_int32)i);
                        /* change the counting from external to internal */
                        ownIncrementReference(pyrdelay->refs[i], VX_INTERNAL);
                        ownDecrementReference(pyrdelay->refs[i], VX_EXTERNAL);
                    }
                }
            }
        }
    }

    return (vx_delay)delay;
}

VX_API_ENTRY vx_status VX_API_CALL vxAgeDelay(vx_delay delay)
{
    vx_status status = VX_SUCCESS;
    if (vxIsValidDelay(delay) == vx_true_e)
    {
        vx_int32 i,j;

        // increment the index
        delay->index = (delay->index + (vx_uint32)delay->count - 1) % (vx_uint32)delay->count;

        VX_PRINT(VX_ZONE_DELAY, "Delay has shifted by 1, base index is now %d\n", delay->index);

        // then reassign the parameters
        for (i = 0; i < (vx_int32)delay->count; i++)
        {
            vx_delay_param_t *param = NULL;

            j = (delay->index + i) % (vx_int32)delay->count;
            param = &delay->set[i];
            do {
                if (param->node != 0)
                {
                    ownNodeSetParameter(param->node,
                                       param->index,
                                       delay->refs[j]);
                }
                param = param->next;
            } while (param != NULL);
        }

        if (delay->type == VX_TYPE_PYRAMID && delay->pyr != NULL)
        {
            // age pyramid levels
            vx_int32 numLevels = (vx_int32)(((vx_pyramid)delay->refs[0])->numLevels);
            for (i = 0; i < numLevels; ++i)
                vxAgeDelay(delay->pyr[i]);
        }
    }
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxRegisterAutoAging(vx_graph graph, vx_delay delay)
{
    unsigned int i;
    vx_status status = VX_SUCCESS;
    vx_bool isAlreadyRegistered = vx_false_e;
    vx_bool isRegisteredDelaysListFull = vx_true_e;

    if (vxIsValidGraph(graph) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (vxIsValidDelay(delay) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    /* check if this particular delay is already registered in the graph */
    for (i = 0; i < VX_INT_MAX_REF; i++)
    {
        if (graph->delays[i] && vxIsValidDelay(graph->delays[i]) && graph->delays[i] == delay)
        {
            isAlreadyRegistered = vx_true_e;
            break;
        }
    }

    /* if not regisered yet, find the first empty slot and register delay */
    if (isAlreadyRegistered == vx_false_e)
    {
        for (i = 0; i < VX_INT_MAX_REF; i++)
        {
            if (vxIsValidDelay(graph->delays[i]) == vx_false_e)
            {
                isRegisteredDelaysListFull = vx_false_e;
                graph->delays[i] = delay;
                break;
            }
        }

        /* report error if there is no empty slots to register delay */
        if (isRegisteredDelaysListFull == vx_true_e)
            status = VX_ERROR_INVALID_REFERENCE;
    }

    return status;
}
