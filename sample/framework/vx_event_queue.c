/*
 * Copyright (c) 2012-2020 The Khronos Group Inc.
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

#ifdef OPENVX_USE_PIPELINING

#include <VX/vx.h>
#include <VX/vx_khr_pipelining.h>
#include <VX/vx_compatibility.h>
#include <string.h>

#include "vx_internal.h"

static vx_uint32 ownEventFindRegistration(vx_context context, vx_reference ref, vx_enum type, vx_uint32 param)
{
    vx_uint32 i;
    for (i = 0; i < context->num_event_reg; i++)
    {
        if (context->event_reg[i].registered == vx_false_e)
            continue;
        if (context->event_reg[i].ref == ref &&
            context->event_reg[i].type == type &&
            context->event_reg[i].param == param)
            return i;
    }
    return (vx_uint32)-1;
}

static vx_bool ownEventMatchesRegistration(vx_context context, const vx_event_t *event)
{
    vx_reference ref = NULL;
    vx_enum type = event->type;
    vx_uint32 param = 0;

    switch (type)
    {
        case VX_EVENT_GRAPH_PARAMETER_CONSUMED:
            ref = (vx_reference)event->event_info.graph_parameter_consumed.graph;
            param = event->event_info.graph_parameter_consumed.graph_parameter_index;
            break;
        case VX_EVENT_GRAPH_COMPLETED:
            ref = (vx_reference)event->event_info.graph_completed.graph;
            break;
        case VX_EVENT_NODE_COMPLETED:
        case VX_EVENT_NODE_ERROR:
            ref = (vx_reference)event->event_info.node_completed.node;
            break;
        case VX_EVENT_USER:
        default:
            return vx_true_e;
    }

    if (ownEventFindRegistration(context, ref, type, param) != (vx_uint32)-1)
        return vx_true_e;

    return vx_false_e;
}

static vx_status ownEventPushLocked(vx_context context, const vx_event_t *event)
{
    if (context->event_count >= VX_INT_MAX_QUEUE_DEPTH)
        return VX_ERROR_NO_RESOURCES;

    vx_int32 idx = context->event_end;
    context->event_queue[idx] = *event;
    context->event_end = (context->event_end + 1) % VX_INT_MAX_QUEUE_DEPTH;
    context->event_count++;
    ownSetEvent(&context->event_ready);
    return VX_SUCCESS;
}

vx_status ownPipelinePostEvent(vx_context context, const vx_event_t *event)
{
    vx_status status = VX_SUCCESS;
    vx_event_t ev = *event;
    if (context->events_enabled == vx_false_e)
        return VX_SUCCESS;

    ownSemWait(&context->event_lock);

    /* If no registration matches a framework event, drop it. User events are always delivered. */
    if (ev.type != VX_EVENT_USER)
    {
        vx_reference ref = NULL;
        vx_uint32 param = 0;
        switch (ev.type)
        {
            case VX_EVENT_GRAPH_PARAMETER_CONSUMED:
                ref = (vx_reference)ev.event_info.graph_parameter_consumed.graph;
                param = ev.event_info.graph_parameter_consumed.graph_parameter_index;
                break;
            case VX_EVENT_GRAPH_COMPLETED:
                ref = (vx_reference)ev.event_info.graph_completed.graph;
                break;
            case VX_EVENT_NODE_COMPLETED:
            case VX_EVENT_NODE_ERROR:
                ref = (vx_reference)ev.event_info.node_completed.node;
                break;
            default:
                break;
        }
        vx_uint32 idx = ownEventFindRegistration(context, ref, ev.type, param);
        if (idx == (vx_uint32)-1)
        {
            ownSemPost(&context->event_lock);
            return VX_SUCCESS;
        }
        ev.app_value = context->event_reg[idx].app_value;
    }

    status = ownEventPushLocked(context, &ev);
    ownSemPost(&context->event_lock);
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxEnableEvents(vx_context context)
{
    if (ownIsValidContext(context) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    ownSemWait(&context->event_lock);
    context->events_enabled = vx_true_e;
    ownSemPost(&context->event_lock);
    return VX_SUCCESS;
}

VX_API_ENTRY vx_status VX_API_CALL vxDisableEvents(vx_context context)
{
    if (ownIsValidContext(context) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    ownSemWait(&context->event_lock);
    context->events_enabled = vx_false_e;
    ownSemPost(&context->event_lock);
    return VX_SUCCESS;
}

VX_API_ENTRY vx_status VX_API_CALL vxSendUserEvent(vx_context context, vx_uint32 id, const void *parameter)
{
    if (ownIsValidContext(context) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (context->events_enabled == vx_false_e)
        return VX_ERROR_NOT_SUPPORTED;

    vx_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = VX_EVENT_USER;
    event.timestamp = ownCaptureTime();
    event.app_value = id;
    event.event_info.user_event.user_event_parameter = parameter;

    return ownPipelinePostEvent(context, &event);
}

VX_API_ENTRY vx_status VX_API_CALL vxWaitEvent(
                    vx_context context, vx_event_t *event,
                    vx_bool do_not_block)
{
    if (ownIsValidContext(context) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;
    if (event == NULL)
        return VX_ERROR_INVALID_PARAMETERS;

    memset(event, 0, sizeof(*event));

    while (1)
    {
        ownSemWait(&context->event_lock);
        vx_bool enabled = context->events_enabled;
        vx_bool has_event = (context->event_count > 0);
        if (has_event == vx_true_e)
        {
            vx_int32 idx = context->event_start;
            *event = context->event_queue[idx];
            context->event_start = (context->event_start + 1) % VX_INT_MAX_QUEUE_DEPTH;
            context->event_count--;
            if (context->event_count == 0)
                ownResetEvent(&context->event_ready);
            ownSemPost(&context->event_lock);
            return VX_SUCCESS;
        }
        ownSemPost(&context->event_lock);

        if (do_not_block == vx_true_e)
        {
            if (enabled == vx_false_e)
                return VX_ERROR_NOT_SUPPORTED;
            return VX_FAILURE;
        }

        if (enabled == vx_false_e)
        {
            /* Block until events are re-enabled and a new event arrives. */
            ownWaitEvent(&context->event_ready, VX_INT_FOREVER);
            ownResetEvent(&context->event_ready);
            continue;
        }

        if (ownWaitEvent(&context->event_ready, VX_INT_FOREVER) == vx_true_e)
        {
            ownResetEvent(&context->event_ready);
            continue;
        }

        return VX_FAILURE;
    }
}

VX_API_ENTRY vx_status VX_API_CALL vxRegisterEvent(vx_reference ref,
                enum vx_event_type_e type, vx_uint32 param, vx_uint32 app_value)
{
    if (ownIsValidReference(ref) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (type != VX_EVENT_GRAPH_PARAMETER_CONSUMED &&
        type != VX_EVENT_GRAPH_COMPLETED &&
        type != VX_EVENT_NODE_COMPLETED &&
        type != VX_EVENT_NODE_ERROR)
        return VX_ERROR_INVALID_PARAMETERS;

    vx_context context = ref->context;
    if (ownIsValidContext(context) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    ownSemWait(&context->event_lock);

    vx_uint32 idx = ownEventFindRegistration(context, ref, type, param);
    if (idx == (vx_uint32)-1)
    {
        if (context->num_event_reg >= VX_INT_MAX_REF)
        {
            ownSemPost(&context->event_lock);
            return VX_ERROR_NO_RESOURCES;
        }
        idx = context->num_event_reg++;
    }

    context->event_reg[idx].registered = vx_true_e;
    context->event_reg[idx].ref = ref;
    context->event_reg[idx].type = type;
    context->event_reg[idx].param = param;
    context->event_reg[idx].app_value = app_value;

    ownSemPost(&context->event_lock);
    return VX_SUCCESS;
}

#endif
