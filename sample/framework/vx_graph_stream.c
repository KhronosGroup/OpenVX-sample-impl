/*

 * Copyright (c) 2012-2026 The Khronos Group Inc.
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

#ifdef OPENVX_USE_STREAMING

#include <VX/vx.h>
#include <VX/vx_khr_pipelining.h>
#include <VX/vx_compatibility.h>

#include "vx_internal.h"

static void ownStreamingResetNodeState(vx_graph graph)
{
    vx_uint32 i;
    for (i = 0; i < graph->numNodes; i++)
    {
        vx_node node = graph->nodes[i];
        if (node == NULL)
            continue;
        node->execution_count = 0;
        /* next execution will determine correct pipeup/steady state */
        node->node_state = VX_NODE_STATE_STEADY;
    }
}

static vx_bool ownNodeBelongsToGraph(vx_graph graph, vx_node node)
{
    vx_uint32 i;
    for (i = 0; i < graph->numNodes; i++)
    {
        if (graph->nodes[i] == node)
            return vx_true_e;
    }
    return vx_false_e;
}

static vx_value_t vxStreamingWorker(void *arg)
{
    vx_graph graph = (vx_graph)arg;
    while (graph->streaming_stop == vx_false_e && graph->streaming_thread_running == vx_true_e)
    {
        vx_status status = vxProcessGraph(graph);
        if (status != VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR, "Streaming graph execution failed with status %d, stopping\n", status);
            break;
        }
        /* yield so a stop request can be observed promptly */
        ownSleepThread(1);
    }
    return 0;
}

VX_API_ENTRY vx_status VX_API_CALL vxEnableGraphStreaming(vx_graph graph,
                vx_node trigger_node)
{
    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (trigger_node != NULL)
    {
        if (ownIsValidSpecificReference(&trigger_node->base, VX_TYPE_NODE) == vx_false_e)
            return VX_ERROR_INVALID_REFERENCE;
        if (trigger_node->graph != graph || ownNodeBelongsToGraph(graph, trigger_node) == vx_false_e)
            return VX_ERROR_INVALID_PARAMETERS;
    }

    graph->streaming_enabled = vx_true_e;
    graph->streaming_trigger_node = trigger_node;

    VX_PRINT(VX_ZONE_GRAPH, "Enabled streaming on graph %p trigger node %p\n", (void *)graph, (void *)trigger_node);
    return VX_SUCCESS;
}

VX_API_ENTRY vx_status VX_API_CALL vxStartGraphStreaming(vx_graph graph)
{
    vx_status status = VX_SUCCESS;

    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (graph->streaming_enabled == vx_false_e)
        return VX_ERROR_NOT_SUPPORTED;

    if (graph->streaming_thread_running == vx_true_e)
        return VX_ERROR_NOT_SUPPORTED;

    if (graph->verified == vx_false_e)
    {
        status = vxVerifyGraph(graph);
        if (status != VX_SUCCESS)
            return status;
    }

    /* reset per-node streaming execution counters so kernels with a pipeup
     * depth observe the pipeup-to-steady transition during this session */
    ownStreamingResetNodeState(graph);

    graph->streaming_stop = vx_false_e;
    graph->streaming_thread_running = vx_true_e;
    graph->streaming_thread = ownCreateThread(vxStreamingWorker, graph);
    if (graph->streaming_thread == 0)
    {
        graph->streaming_thread_running = vx_false_e;
        return VX_FAILURE;
    }

    VX_PRINT(VX_ZONE_GRAPH, "Started streaming on graph %p\n", (void *)graph);
    return VX_SUCCESS;
}

VX_API_ENTRY vx_status VX_API_CALL vxStopGraphStreaming(vx_graph graph)
{
    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (graph->streaming_thread_running == vx_false_e)
        return VX_ERROR_NOT_SUPPORTED;

    graph->streaming_stop = vx_true_e;

    if (graph->streaming_thread)
    {
        ownJoinThread(graph->streaming_thread, NULL);
        graph->streaming_thread = 0;
    }

    graph->streaming_thread_running = vx_false_e;
    graph->streaming_enabled = vx_false_e;
    graph->streaming_trigger_node = NULL;

    VX_PRINT(VX_ZONE_GRAPH, "Stopped streaming on graph %p\n", (void *)graph);
    return VX_SUCCESS;
}

#endif
