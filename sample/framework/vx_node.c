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

#include <ctype.h>
#include "vx_internal.h"
#include "vx_node.h"

void ownPrintNode(vx_node_t *node)
{
    if (node)
        VX_PRINT(VX_ZONE_NODE, "vx_node_t:%p %s:%d affinity:%s\n",
            node,
            node->kernel->name,
            node->kernel->enumeration,
            node->base.context->targets[node->affinity].name);
}

void ownNodeSetParameter(vx_node node, vx_uint32 index, vx_reference value)
{
    if (node->parameters[index]) {
        ownReleaseReferenceInt(&node->parameters[index], node->parameters[index]->type, VX_INTERNAL, NULL);
    }

    ownIncrementReference(value, VX_INTERNAL);
    node->parameters[index] = (vx_reference_t *)value;
}

VX_API_ENTRY vx_node VX_API_CALL vxCreateGenericNode(vx_graph g, vx_kernel k)
{
    vx_node_t *node = NULL;
    vx_graph_t *graph = (vx_graph_t *)g;
    vx_kernel_t *kernel = (vx_kernel_t *)k;

    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e)
    {
        if (ownIsValidSpecificReference(&kernel->base, VX_TYPE_KERNEL) == vx_true_e)
        {
            vx_uint32 n = 0;
            ownSemWait(&graph->base.lock);
            for (n = 0; n < VX_INT_MAX_REF; n++)
            {
                if (graph->nodes[n] == NULL)
                {
                    node = (vx_node)ownCreateReference(graph->base.context, VX_TYPE_NODE, VX_EXTERNAL, &graph->base);
                    if (vxGetStatus((vx_reference)node) == VX_SUCCESS && node->base.type == VX_TYPE_NODE)
                    {
                        /* reference the abstract kernel. */
                        node->kernel = kernel;
                        node->affinity = kernel->affinity;

                        /* show that there are potentially multiple nodes using this kernel. */
                        ownIncrementReference(&kernel->base, VX_INTERNAL);

                        /* copy the attributes over */
                        memcpy(&node->attributes, &kernel->attributes, sizeof(vx_kernel_attr_t));

                        /* setup our forward and back references to the node/graph */
                        graph->nodes[n] = node;
                        node->graph = graph;
                        ownIncrementReference(&node->base, VX_INTERNAL); /* one for the graph */

                        /* increase the count of nodes in the graph. */
                        graph->numNodes++;

                        ownInitPerf(&graph->nodes[n]->perf);

                        /* force a re-verify */
                        graph->reverify = graph->verified;
                        graph->verified = vx_false_e;
                        graph->state = VX_GRAPH_STATE_UNVERIFIED;

                        VX_PRINT(VX_ZONE_NODE, "Created Node %p %s affinity:%s\n", node, node->kernel->name, node->base.context->targets[node->affinity].name);
                    }
                    break; /* succeed or fail, break. */
                }
            }
            ownSemPost(&graph->base.lock);
            ownPrintReference((vx_reference )node);
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Kernel %p was invalid!\n", kernel);
            vxAddLogEntry((vx_reference)g, VX_ERROR_INVALID_REFERENCE, "Kernel %p was invalid!\n", kernel);
            node = (vx_node_t *)ownGetErrorObject(graph->base.context, VX_ERROR_INVALID_REFERENCE);
        }
    }
    else {
        VX_PRINT(VX_ZONE_ERROR, "Graph %p was invalid!\n", graph);
        vxAddLogEntry((vx_reference)g, VX_ERROR_INVALID_REFERENCE, "Graph %p as invalid!\n", graph);
    }

    return (vx_node)node;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryNode(vx_node n, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    vx_node_t *node = (vx_node_t *)n;
    if (ownIsValidSpecificReference(&node->base, VX_TYPE_NODE) == vx_true_e)
    {
        switch (attribute)
        {
            case VX_NODE_PERFORMANCE:
                if (VX_CHECK_PARAM(ptr, size, vx_perf_t, 0x3))
                {
                    memcpy(ptr, &node->perf, size);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_STATUS:
                if (VX_CHECK_PARAM(ptr, size, vx_status, 0x3))
                {
                    *(vx_status *)ptr = node->status;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_LOCAL_DATA_SIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = node->attributes.localDataSize;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_LOCAL_DATA_PTR:
                if (VX_CHECK_PARAM(ptr, size, vx_ptr_t, 0x3))
                {
                    *(vx_ptr_t *)ptr = node->attributes.localDataPtr;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
#ifdef OPENVX_KHR_NODE_MEMORY
            case VX_NODE_GLOBAL_DATA_SIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = node->attributes.globalDataSize;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_GLOBAL_DATA_PTR:
                if (VX_CHECK_PARAM(ptr, size, vx_ptr_t, 0x3))
                {
                    *(vx_ptr_t *)ptr = node->attributes.globalDataPtr;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
#endif
            case VX_NODE_BORDER:
                if (VX_CHECK_PARAM(ptr, size, vx_border_t, 0x3))
                {
                    VX_PRINT(VX_ZONE_NODE, "Border mode %x set!\n", node->attributes.borders.mode);
                    memcpy((vx_border_t *)ptr, &node->attributes.borders, sizeof(vx_border_t));
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
#ifdef OPENVX_KHR_TILING
            case VX_NODE_INPUT_NEIGHBORHOOD:
                if (VX_CHECK_PARAM(ptr, size, vx_neighborhood_size_t, 0x3))
                {
                    memcpy(ptr, &node->kernel->attributes.nhbdinfo, size);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_OUTPUT_TILE_BLOCK_SIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_tile_block_size_t, 0x3))
                {
                    memcpy(ptr, &node->kernel->attributes.blockinfo, size);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_TILE_MEMORY_SIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = node->attributes.tileDataSize;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
#endif
            case VX_NODE_PARAMETERS:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    vx_uint32 numParams = node->kernel->signature.num_parameters;
                    VX_PRINT(VX_ZONE_NODE, "Number of node parameters is %d\n", numParams);
                    memcpy((vx_uint32*)ptr, &numParams, sizeof(numParams));
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_IS_REPLICATED:
                if (VX_CHECK_PARAM(ptr, size, vx_bool, 0x3))
                {
                    vx_bool is_replicated = node->is_replicated;
                    if (vx_true_e == is_replicated)
                        VX_PRINT(VX_ZONE_NODE, "Node is replicated\n");
                    else
                        VX_PRINT(VX_ZONE_NODE, "Number is not replicated\n");
                    memcpy((vx_bool*)ptr, &is_replicated, sizeof(is_replicated));
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_NODE_REPLICATE_FLAGS:
            {
                vx_size sz = sizeof(vx_bool)*node->kernel->signature.num_parameters;
                if (size == sz && ((vx_size)ptr & 0x3) == 0)
                {
                    vx_uint32 i = 0;
                    vx_uint32 numParams = node->kernel->signature.num_parameters;
                    for (i = 0; i < numParams; i++)
                        ((vx_bool*)ptr)[i] = node->replicated_flags[i];
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
            }
                break;
            case VX_NODE_VALID_RECT_RESET:
                if (VX_CHECK_PARAM(ptr, size, vx_bool, 0x3))
                {
                    vx_bool valid_rect_reset = node->attributes.valid_rect_reset;
                    if (vx_true_e == valid_rect_reset)
                        VX_PRINT(VX_ZONE_NODE, "Valid rect to be reset to full image\n");
                    else
                        VX_PRINT(VX_ZONE_NODE, "Valid rect to be calculated\n");
                    memcpy((vx_bool*)ptr, &valid_rect_reset, sizeof(valid_rect_reset));
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
    VX_PRINT(VX_ZONE_API, "Returned %d\n", status);
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetNodeAttribute(vx_node n, vx_enum attribute, const void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    vx_node_t *node = (vx_node_t *)n;
    if (ownIsValidSpecificReference(&node->base, VX_TYPE_NODE) == vx_true_e)
    {
        if (node->graph->verified == vx_true_e)
        {
            return VX_ERROR_NOT_SUPPORTED;
        }
        switch (attribute)
        {
            case VX_NODE_LOCAL_DATA_SIZE:
                if (node->local_data_change_is_enabled)
                {
                    if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                    {
                        node->attributes.localDataSize = *(vx_size *)ptr;
                        node->local_data_set_by_implementation = vx_false_e;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                    }
                }
                else
                {
                    status = VX_ERROR_NOT_SUPPORTED;
                }
                break;
            case VX_NODE_LOCAL_DATA_PTR:
                if (node->local_data_change_is_enabled)
                {
                    if (VX_CHECK_PARAM(ptr, size, vx_ptr_t, 0x3))
                    {
                        node->attributes.localDataPtr = *(vx_ptr_t *)ptr;
                        node->local_data_set_by_implementation = vx_false_e;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                    }
                }
                else
                {
                    status = VX_ERROR_NOT_SUPPORTED;
                }
                break;
            case VX_NODE_BORDER:
                if (VX_CHECK_PARAM(ptr, size, vx_border_t, 0x3))
                {
#ifdef OPENVX_KHR_TILING
                    if (node->attributes.borders.mode == VX_BORDER_MODE_SELF)
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                    else
#endif
                    {
                        memcpy(&node->attributes.borders, (vx_border_t *)ptr, sizeof(vx_border_t));
                    }
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
    return status;
}

void ownDestructNode(vx_reference ref)
{
    vx_uint32 p = 0;
    vx_node node = (vx_node)ref;

    if (node->kernel == NULL)
    {
        VX_PRINT(VX_ZONE_WARNING, "Node has no kernel!\n");
        DEBUG_BREAK();
    }

    /* de-initialize the kernel */
    if (node->kernel->deinitialize)
    {
        vx_status status;
        if ((node->kernel->user_kernel == vx_true_e) && (node->local_data_set_by_implementation == vx_false_e))
            node->local_data_change_is_enabled = vx_true_e;
        status = node->kernel->deinitialize((vx_node)node,
                                            (vx_reference *)node->parameters,
                                            node->kernel->signature.num_parameters);
        node->local_data_change_is_enabled = vx_false_e;
        if (status != VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR,"Failed to de-initialize kernel %s!\n", node->kernel->name);
        }
    }

    /* remove, don't delete, all references from the node itself */
    for (p = 0; p < node->kernel->signature.num_parameters; p++)
    {
        vx_reference ref = node->parameters[p];
        if (ref)
        {
            /* Remove the potential delay association */
            if (ref->delay!=NULL) {
                vx_bool res = ownRemoveAssociationToDelay(ref, node, p);
                if (res == vx_false_e) {
                    VX_PRINT(VX_ZONE_ERROR, "Internal error removing delay association\n");
                }
            }
            ownReleaseReferenceInt(&ref, ref->type, VX_INTERNAL, NULL);
            node->parameters[p] = NULL;
        }
    }

    /* free the local memory */
    if (node->attributes.localDataPtr)
    {
        free(node->attributes.localDataPtr);
        node->attributes.localDataPtr = NULL;
    }

    ownReleaseReferenceInt((vx_reference *)&node->kernel, VX_TYPE_KERNEL, VX_INTERNAL, NULL);
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseNode(vx_node *n)
{
    return ownReleaseReferenceInt((vx_reference *)n, VX_TYPE_NODE, VX_EXTERNAL, NULL);
}

vx_status ownRemoveNodeInt(vx_node *n)
{
    vx_node_t *node = (vx_node_t *)(n?*n:0);
    vx_status status =  VX_ERROR_INVALID_REFERENCE;
    if (node && ownIsValidSpecificReference(&node->base, VX_TYPE_NODE))
    {
        if (node->graph)
        {
            vx_uint32 i = 0;
            vx_bool removedFromGraph = vx_false_e;
            ownSemWait(&node->graph->base.lock);
            /* remove the reference from the graph */
            for (i = 0; i < node->graph->numNodes; i++)
            {
                if (node->graph->nodes[i] == node)
                {
                    node->graph->numNodes--;
                    node->graph->nodes[i] = node->graph->nodes[node->graph->numNodes];
                    node->graph->nodes[node->graph->numNodes] = NULL;
                    /* force the graph to be verified again */
                    node->graph->reverify = node->graph->verified;
                    node->graph->verified = vx_false_e;
                    node->graph->state = VX_GRAPH_STATE_UNVERIFIED;
                    removedFromGraph = vx_true_e;
                    break;
                }
            }
            ownSemPost(&node->graph->base.lock);
            /* If this node is within a graph, release internal reference to graph */
            if(removedFromGraph) {
                ownReleaseReferenceInt((vx_reference *)&node, VX_TYPE_NODE, VX_INTERNAL, NULL);
                status = VX_SUCCESS;
            }
        }
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxRemoveNode(vx_node *n)
{
    vx_node_t *node = (vx_node_t *)(n?*n:0);
    vx_status status =  VX_ERROR_INVALID_REFERENCE;
    if (node && ownIsValidSpecificReference(&node->base, VX_TYPE_NODE))
    {
        status = ownRemoveNodeInt(n);
        if(status == VX_SUCCESS) {
            status = ownReleaseReferenceInt((vx_reference *)&node, VX_TYPE_NODE, VX_EXTERNAL, NULL);
            if(status == VX_SUCCESS) {
                *n = NULL;
            }
        }
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxAssignNodeCallback(vx_node n, vx_nodecomplete_f callback)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    vx_node_t *node = (vx_node_t *)n;
    if (ownIsValidSpecificReference(&node->base, VX_TYPE_NODE) == vx_true_e)
    {
        if ((callback) && (node->callback))
        {
            VX_PRINT(VX_ZONE_ERROR, "Attempting to overriding existing callback %p on Node %s!\n", node->callback, node->kernel->name);
            status = VX_ERROR_NOT_SUPPORTED;
        }
        else
        {
            node->callback = callback;
            status = VX_SUCCESS;
        }
    }
    return status;
}

VX_API_ENTRY vx_nodecomplete_f VX_API_CALL vxRetrieveNodeCallback(vx_node n)
{
    vx_node_t *node = (vx_node_t *)n;
    vx_nodecomplete_f cb = NULL;
    if (ownIsValidSpecificReference(&node->base, VX_TYPE_NODE) == vx_true_e)
    {
        cb = node->callback;
    }
    return cb;
}

VX_API_ENTRY vx_status VX_API_CALL vxReplicateNode(vx_graph graph, vx_node first_node, vx_bool replicate[], vx_uint32 number_of_parameters)
{
    vx_uint32 n;
    vx_uint32 p;
    vx_uint32 numParams = 0;
    vx_size   num_of_replicas = 0;
    vx_status status = VX_SUCCESS;

    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) != vx_true_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Graph %p was invalid!\n", graph);
        vxAddLogEntry((vx_reference)graph, VX_ERROR_INVALID_REFERENCE, "Graph %p as invalid!\n", graph);
		status = VX_ERROR_INVALID_REFERENCE;
    }
    else if (ownIsValidSpecificReference(&first_node->base, VX_TYPE_NODE) != vx_true_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Node %p was invalid!\n", first_node);
        vxAddLogEntry((vx_reference)first_node, VX_ERROR_INVALID_REFERENCE, "Node %p as invalid!\n", first_node);
		status = VX_ERROR_INVALID_REFERENCE;
    }
    else if (first_node->graph != graph)
		status = VX_FAILURE;
    else if (replicate == NULL)
		status = VX_ERROR_INVALID_PARAMETERS;
	else
	{
		/* validate replicated params */
		status = vxQueryNode(first_node, VX_NODE_PARAMETERS, &numParams, sizeof(numParams));
		if (VX_SUCCESS == status)
		{
			if (numParams != number_of_parameters)
				status = VX_ERROR_INVALID_PARAMETERS;
		}

		for (p = 0; (VX_SUCCESS == status) && p < number_of_parameters; p++)
		{
			vx_parameter param = 0;
			vx_reference ref = 0;
			vx_enum type = 0;
			vx_enum state = 0;
			vx_enum dir = 0;

			param = vxGetParameterByIndex(first_node, p);

			vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(vx_enum));
			vxQueryParameter(param, VX_PARAMETER_REF, &ref, sizeof(vx_reference));
			vxQueryParameter(param, VX_PARAMETER_STATE, &state, sizeof(vx_enum));
			vxQueryParameter(param, VX_PARAMETER_DIRECTION, &dir, sizeof(vx_enum));

			if (replicate[p] == vx_false_e && (dir == VX_OUTPUT || dir == VX_BIDIRECTIONAL))
				status = VX_FAILURE;

			if (replicate[p] == vx_true_e)
			{
				if (ownIsValidSpecificReference(ref, type) == vx_true_e)
				{
					vx_size items = 0;
					if (ownIsValidSpecificReference(ref->scope, VX_TYPE_PYRAMID) == vx_true_e)
					{
						vx_pyramid pyramid = (vx_pyramid)ref->scope;
						vxQueryPyramid(pyramid, VX_PYRAMID_LEVELS, &items, sizeof(vx_size));
					}
					else if (ownIsValidSpecificReference(ref->scope, VX_TYPE_OBJECT_ARRAY) == vx_true_e)
					{
						vx_object_array object_array = (vx_object_array)ref->scope;
						vxQueryObjectArray(object_array, VX_OBJECT_ARRAY_NUMITEMS, &items, sizeof(vx_size));
					}
					else
						status = VX_FAILURE;

					if (num_of_replicas == 0)
						num_of_replicas = items;

					if (num_of_replicas != 0 && items != num_of_replicas)
						status = VX_FAILURE;
				}
				else
					status = VX_FAILURE;
			}

			vxReleaseReference(&ref);
			vxReleaseParameter(&param);
		}
	}
	
	if (VX_SUCCESS == status)
	{
		/* set replicate flag for node */
		first_node->is_replicated = vx_true_e;

		for (n = 0; n < number_of_parameters; n++)
		{
			first_node->replicated_flags[n] = replicate[n];
		}
	}
    
	return status;
}


// ![SAMPLE EXTENSION]
vx_status ownSetChildGraphOfNode(vx_node node, vx_graph graph)
{
    vx_status status = VX_ERROR_INVALID_GRAPH;

    vx_bool valid_node = ownIsValidSpecificReference(&node->base, VX_TYPE_NODE);

    if ((valid_node == vx_true_e) && graph &&
        (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e))
    {
        vx_uint32 p, num = node->kernel->signature.num_parameters;
        /* check to make sure the signature of the node matches the signature of the graph. */
        //if (node->kernel->signature.numParams == graph->numParams)
        if (graph->numParams > 0)
        {
            vx_bool match = vx_true_e;
            for (p = 0; p < num; p++)
            {
                vx_uint32 child_index = graph->parameters[p].index;
                if (graph->parameters[p].node)
                {
                    if ((node->kernel->signature.directions[p] != graph->parameters[p].node->kernel->signature.directions[child_index]) ||
                        (node->kernel->signature.states[p] != graph->parameters[p].node->kernel->signature.states[child_index]) ||
                        (node->kernel->signature.types[p] != graph->parameters[p].node->kernel->signature.types[child_index]))
                    {
                        vxAddLogEntry(&graph->base, VX_ERROR_INVALID_PARAMETERS,
                                "Graph parameter %u does not match node parameter %u\n"
                                "\t%s[%u] dir:%d req:%d type:0%x\n"
                                "\t%s[%u] dir:%d req:%d type:0%x\n",
                                p, p,
                                node->kernel->name,
                                p,
                                node->kernel->signature.directions[p],
                                node->kernel->signature.states[p],
                                node->kernel->signature.types[p],
                                graph->parameters[p].node->kernel->name,
                                p,
                                graph->parameters[p].node->kernel->signature.directions[child_index],
                                graph->parameters[p].node->kernel->signature.states[child_index],
                                graph->parameters[p].node->kernel->signature.types[child_index]);
                        match = vx_false_e;
                    }
                }
                else
                {
                    vxAddLogEntry(&graph->base, VX_ERROR_INVALID_NODE, "WARNING: the child graph has declared graph parameter index %u node reference to be NULL!\n", p);
                }
            }
            if (match == vx_true_e)
            {
                node->child = graph;
                ownIncrementReference(&graph->base, VX_INTERNAL);
                VX_PRINT(VX_ZONE_GRAPH, "Graph "VX_FMT_REF" set as child graph of "VX_FMT_REF"\n", graph, node->graph);
                status = VX_SUCCESS;
            }
        }
        else
        {
            //vxAddLogEntry((vx_reference)child, VX_ERROR_INVALID_GRAPH, "Number of parameters on graph does not equal number of parameters on node!");
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_GRAPH, "Graph must have some parameters!");
        }
    }
    else if ((valid_node == vx_true_e) && (graph == NULL) && (node->child != NULL))
    {
        status = ownReleaseReferenceInt((vx_reference *)&node->child, VX_TYPE_GRAPH, VX_INTERNAL, NULL);
    }

    return status;
}

vx_graph ownGetChildGraphOfNode(vx_node n)
{
    vx_node_t *node = (vx_node_t *)n;
    vx_graph_t *graph = 0;
    if (ownIsValidSpecificReference(&node->base, VX_TYPE_NODE) == vx_true_e)
    {
        graph = node->child;
    }
    return (vx_graph)graph;
}

// ![SAMPLE EXTENSION]

static vx_kernel_t* findKernelByEnum(vx_target_t* target, vx_enum enumeration)
{
    vx_uint32 k = 0;
    for (k = 0; k < target->num_kernels; k++)
    {
        if (target->kernels[k].enumeration == enumeration)
        {
            return &target->kernels[k];
        }
    }
    return NULL;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetNodeTarget(vx_node node, vx_enum target_enum, const char* target_string)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (ownIsValidSpecificReference(&node->base, VX_TYPE_NODE) == vx_true_e)
    {
        vx_context_t *context = node->base.context;
        vx_kernel_t *kernel = NULL;
        vx_uint32 rt = 0;
        vx_uint32 t = 0;
        switch (target_enum)
        {
            case VX_TARGET_ANY:
                for (t = 0; (t < context->num_targets) && (kernel == NULL); t++)
                {
                    rt = t; /*context->priority_targets[t];*/
                    kernel = findKernelByEnum(&context->targets[rt], node->kernel->enumeration);
                }
                break;

            case VX_TARGET_STRING:
                {
                    size_t len = strlen(target_string);
                    char* target_lower_string = (char*)calloc(len + 1, sizeof(char));
                    if (target_lower_string)
                    {
                        unsigned int i;
                        // to lower case
                        for (i = 0; target_string[i] != 0; i++)
                        {
                            target_lower_string[i] = (char)tolower(target_string[i]);
                        }

                        for (t = 0; (t < context->num_targets) && (kernel == NULL); t++)
                        {
                            rt = context->priority_targets[t];
                            if (ownMatchTargetNameWithString(context->targets[rt].name, target_lower_string) == vx_true_e)
                            {
                                kernel = findKernelByEnum(&context->targets[rt], node->kernel->enumeration);
                            }
                        }

                        free(target_lower_string);
                    }
                }
                break;

            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
        if (kernel != NULL) /* target/kernel were found */
        {
            ownDecrementReference(&node->kernel->base, VX_INTERNAL);
            node->kernel = kernel;
            ownIncrementReference(&node->kernel->base, VX_INTERNAL);

            node->affinity = rt;
            node->graph->reverify = node->graph->verified;
            node->graph->verified = vx_false_e;
            node->graph->state = VX_GRAPH_STATE_UNVERIFIED;
            status = VX_SUCCESS;
        }
        else /* target/kernel were not found */
        {
            status = VX_ERROR_NOT_SUPPORTED;
        }
    }
    return status;
}


