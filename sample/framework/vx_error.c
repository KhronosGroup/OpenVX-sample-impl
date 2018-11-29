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
#include "vx_error.h"

void ownReleaseErrorInt(vx_error_t **error)
{
    ownReleaseReferenceInt((vx_reference *)error, VX_TYPE_ERROR, VX_INTERNAL, NULL);
}

vx_error_t *ownAllocateError(vx_context_t *context, vx_status status)
{
    /* PROBLEM: ownCreateReference needs error object to be created already */
    vx_error_t *error = (vx_error_t *)ownCreateReference(context, VX_TYPE_ERROR, VX_INTERNAL, &context->base);
    if (error)
    {
        error->status = status;
    }
    return error;
}

vx_bool ownCreateConstErrors(vx_context_t *context)
{
    vx_bool ret = vx_true_e;
    vx_enum e = 0;
    /* create an error object for each status enumeration */
    for (e = VX_STATUS_MIN; (e < VX_SUCCESS) && (ret == vx_true_e); e++)
    {
        if (ownAllocateError(context, e) == NULL)
            ret = vx_false_e;
    }
    return ret;
}

vx_error_t *ownGetErrorObject(vx_context_t *context, vx_status status)
{
    vx_error_t *error = NULL;
    vx_size i = 0ul;
    ownSemWait(&context->base.lock);
    for (i = 0ul; i < context->num_references; i++)
    {
        if (context->reftable[i] == NULL)
            continue;

        if (context->reftable[i]->type == VX_TYPE_ERROR)
        {
            error = (vx_error_t *)context->reftable[i];
            if (error->status == status)
            {
                break;
            }
            error = NULL;
        }
    }
    ownSemPost(&context->base.lock);
    return error;
}

VX_API_ENTRY vx_status VX_API_CALL vxGetStatus(vx_reference reference)
{
    vx_reference_t *ref = (vx_reference_t *)reference;
    if (ref == NULL)
    {
        /*! \internal probably ran out of handles or memory */
        return VX_ERROR_NO_RESOURCES;
    }
    else if (ownIsValidReference(ref) == vx_true_e)
    {
        if (ref->type == VX_TYPE_ERROR)
        {
            vx_error_t *error = (vx_error_t *)ref;
            return error->status;
        }
        else
        {
            return VX_SUCCESS;
        }
    }
    else if (ownIsValidContext((vx_context_t *)ref) == vx_true_e)
    {
        return VX_SUCCESS;
    }
    return VX_FAILURE;
}
