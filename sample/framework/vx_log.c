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
#include "vx_log.h"

VX_API_ENTRY void VX_API_CALL vxRegisterLogCallback(vx_context context, vx_log_callback_f callback, vx_bool reentrant)
{
    vx_context_t *cntxt = (vx_context_t *)context;
    if (ownIsValidContext(cntxt) == vx_true_e)
    {
        ownSemWait(&cntxt->base.lock);
        if ((cntxt->log_callback == NULL) && (callback != NULL))
        {
            cntxt->log_enabled = vx_true_e;
            if (reentrant == vx_false_e)
            {
                ownCreateSem(&cntxt->log_lock, 1);
            }
            cntxt->log_reentrant = reentrant;
        }
        if ((cntxt->log_callback != NULL) && (callback == NULL))
        {
            if (cntxt->log_reentrant == vx_false_e)
            {
                ownDestroySem(&cntxt->log_lock);
            }
            cntxt->log_enabled = vx_false_e;
        }
        if ((cntxt->log_callback != NULL) && (callback != NULL) && (cntxt->log_callback != callback))
        {
            if (cntxt->log_reentrant == vx_false_e)
            {
                ownDestroySem(&cntxt->log_lock);
            }
            if (reentrant == vx_false_e)
            {
                ownCreateSem(&cntxt->log_lock, 1);
            }
            cntxt->log_reentrant = reentrant;
        }
        cntxt->log_callback = callback;
        ownSemPost(&cntxt->base.lock);
    }
}

VX_API_ENTRY void VX_API_CALL vxAddLogEntry(vx_reference ref, vx_status status, const char *message, ...)
{
    va_list ap;
    vx_context_t *context = NULL;
    vx_char string[VX_MAX_LOG_MESSAGE_LEN];

    if (ownIsValidReference(ref) == vx_false_e)
    {
        if (ownIsValidContext((vx_context_t *)ref) == vx_false_e)
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid reference!\n");
            return;
        }
    }

    if (status == VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid status code!\n");
        return;
    }

    if (message == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid message!\n");
        return;
    }

    if (ref->type == VX_TYPE_CONTEXT)
    {
        context = (vx_context_t *)ref;
    }
    else
    {
        context = ref->context;
    }

    if (context->log_callback == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "No callback is registered\n");
        return;
    }

    if (context->log_enabled == vx_false_e)
    {
        return;
    }

    va_start(ap, message);
    vsnprintf(string, VX_MAX_LOG_MESSAGE_LEN, message, ap);
    string[VX_MAX_LOG_MESSAGE_LEN-1] = 0; // for MSVC which is not C99 compliant
    va_end(ap);
    if (context->log_reentrant == vx_false_e)
        ownSemWait(&context->log_lock);
    context->log_callback(context, ref, status, string);
    if (context->log_reentrant == vx_false_e)
        ownSemPost(&context->log_lock);
    return;
}

