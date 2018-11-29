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
#include "vx_graph.h"

static vx_uint32 vxNextNode(vx_graph graph, vx_uint32 index)
{
    return ((index + 1) % graph->numNodes);
}

static vx_reference vxLocateBaseLocation(vx_reference ref, vx_size* start, vx_size* end)
{
	if (ref->type == VX_TYPE_IMAGE)
	{
		start[0] = start[1] = 0;
		end[0] = ((vx_image)ref)->width;
		end[1] = ((vx_image)ref)->height;
	}
	else
	{
		for (vx_uint32 i = 0; i < VX_MAX_TENSOR_DIMENSIONS; i++)
		{
			start[i] = 0;
			end[i] = ((vx_tensor)ref)->dimensions[i];
		}
	}
	while ((ref->type == VX_TYPE_IMAGE && ((vx_image)ref)->parent && ((vx_image)ref)->parent != ((vx_image)ref)) 
		||
		(ref->type == VX_TYPE_TENSOR && ((vx_tensor)ref)->parent && ((vx_tensor)ref)->parent != ((vx_tensor)ref))
		)
	{
		if (ref->type == VX_TYPE_IMAGE)
		{
			vx_image img = (vx_image)ref;
			vx_size plane_offset = img->memory.ptrs[0] - img->parent->memory.ptrs[0];
			vx_uint32 dy = (vx_uint32)(plane_offset * img->scale[0][VX_DIM_Y] / img->memory.strides[0][VX_DIM_Y]);
			vx_uint32 dx = (vx_uint32)((plane_offset - (dy * img->memory.strides[0][VX_DIM_Y] / img->scale[0][VX_DIM_Y])) * img->scale[0][VX_DIM_X] / img->memory.strides[0][VX_DIM_X]);
			start[0] += dx;
			end[0] += dx;
			start[1] += dy;
			end[1] += dy;
			ref = (vx_reference)img->parent;
		}
		else
		{
			vx_tensor tensor = (vx_tensor)ref;
			vx_uint32 offset = 0;
			for (vx_int32 i = tensor->number_of_dimensions - 1; i >= 0; i--)
			{
				start[i] = ((vx_uint8*)tensor->addr - (vx_uint8*)tensor->parent->addr - offset) / tensor->stride[i];
				end[i] = start[i] + tensor->dimensions[i];
				offset += (vx_uint32)(start[i] * tensor->stride[i]);
			}
			ref = (vx_reference)tensor->parent;
		}
	}
	return ref;
}
static vx_tensor vxLocateView(vx_tensor mddata, vx_size* start, vx_size* end)
{
    for (vx_uint32 i = 0; i < VX_MAX_TENSOR_DIMENSIONS; i++)
    {
        start[i] = 0;
        end[i] = mddata->dimensions[i];
    }
    while (mddata->parent && mddata->parent != mddata)
    {
        size_t offset = 0;
        for (vx_int32 i = mddata->number_of_dimensions-1; i >= 0; i--)
        {
            start[i] = ((vx_uint8*)mddata->addr - (vx_uint8*)mddata->parent->addr - offset) / mddata->stride[i];
            end[i] = start[i] + mddata->dimensions[i];
            offset += start[i] * mddata->stride[i];
        }
        mddata = mddata->parent;
    }
    return mddata;
}

static vx_bool vxCheckWriteDependency(vx_reference ref1, vx_reference ref2)
{
    if (!ref1 || !ref2) // garbage input
        return vx_false_e;

    if (ref1 == ref2)
        return vx_true_e;

    // write to layer then read pyramid
    if (ref1->type == VX_TYPE_PYRAMID && ref2->type == VX_TYPE_IMAGE)
    {
        vx_image img = (vx_image)ref2;
        while (img->parent && img->parent != img) img = img->parent;
        if (img->base.scope == ref1)
            return vx_true_e;
    }

    // write to pyramid then read a layer
    if (ref2->type == VX_TYPE_PYRAMID && ref1->type == VX_TYPE_IMAGE)
    {
        vx_image img = (vx_image)ref1;
        while (img->parent && img->parent != img) img = img->parent;
        if (img->base.scope == ref2)
            return vx_true_e;
    }

    // two images or ROIs
    if (ref1->type == VX_TYPE_IMAGE && ref2->type == VX_TYPE_IMAGE)
    {
		vx_size rr_start[VX_MAX_TENSOR_DIMENSIONS], rw_start[VX_MAX_TENSOR_DIMENSIONS], rr_end[VX_MAX_TENSOR_DIMENSIONS], rw_end[VX_MAX_TENSOR_DIMENSIONS];
		vx_reference refr = vxLocateBaseLocation(ref1, rr_start, rr_end);
		vx_reference refw = vxLocateBaseLocation(ref2, rw_start, rw_end);
		if (refr == refw)
        {
			if (refr->type == VX_TYPE_IMAGE)
			{
            // check for ROI intersection
				if (rr_start[0] < rw_end[0] && rr_end[0] > rw_start[0] && rr_start[1] < rw_end[1] && rr_end[1] > rw_start[1])
                return vx_true_e;
        }
			else
			{
				if (refr->type == VX_TYPE_TENSOR)
				{
					for (vx_uint32 i = 0; i < ((vx_tensor)refr)->number_of_dimensions; i++)
					{
						if ((rr_start[i] >= rw_end[i]) ||
							(rw_start[i] >= rr_end[i]))
						{
							return vx_false_e;
						}
					}
					return vx_true_e;
    }
			}
		}
    }
    if (ref1->type == VX_TYPE_TENSOR && ref2->type == VX_TYPE_TENSOR)
    {
        vx_size rr_start[VX_MAX_TENSOR_DIMENSIONS], rw_start[VX_MAX_TENSOR_DIMENSIONS], rr_end[VX_MAX_TENSOR_DIMENSIONS], rw_end[VX_MAX_TENSOR_DIMENSIONS];
        vx_tensor datar = vxLocateView((vx_tensor)ref1, rr_start, rr_end);
        vx_tensor dataw = vxLocateView((vx_tensor)ref2, rw_start, rw_end);
        if (datar == dataw)
        {
            for (vx_uint32 i = 0; i < datar->number_of_dimensions; i++)
            {
                if ((rr_start[i] >= rw_end[i]) ||
                    (rw_start[i] >= rr_end[i]))
                {
                    return vx_false_e;
                }
            }
                return vx_true_e;
        }
    }

    return vx_false_e;
}

/*! \brief This function starts on the next node in the list and loops until we
 * hit the original node again. Parse over the nodes in circular fashion.
 */
vx_status ownFindNodesWithReference(vx_graph graph,
                                   vx_reference ref,
                                   vx_uint32 refnodes[],
                                   vx_uint32 *count,
                                   vx_enum reftype)
{
    vx_uint32 n, p, nc = 0, max;
    vx_status status = VX_ERROR_INVALID_LINK;

    /* save the maximum number of nodes to find */
    max = *count;

    /* reset the current count to zero */
    *count = 0;

    VX_PRINT(VX_ZONE_GRAPH,"Find nodes with reference "VX_FMT_REF" type %d over %u nodes upto %u finds\n", ref, reftype, graph->numNodes, max);
    for (n = 0; n < graph->numNodes; n++)
    {
        for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
        {
            vx_enum dir = graph->nodes[n]->kernel->signature.directions[p];
            vx_reference_t *thisref = graph->nodes[n]->parameters[p];

            /* VX_PRINT(VX_ZONE_GRAPH,"\tchecking node[%u].parameter[%u] dir = %d ref = "VX_FMT_REF" (=?%d:"VX_FMT_REF")\n", n, p, dir, thisref, reftype, ref); */
            if ((dir == reftype) && vxCheckWriteDependency(thisref, ref))
            {
                if (nc < max)
                {
                    VX_PRINT(VX_ZONE_GRAPH, "match at node[%u].parameter[%u]\n", n, p);
                    if (refnodes)
                        refnodes[nc] = n;
                    nc++;
                    status = VX_SUCCESS;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "ERROR: Overflow in refnodes[]\n");
                }
            }
        }
    }
    *count = nc;
    VX_PRINT(VX_ZONE_GRAPH, "Found %u nodes with reference "VX_FMT_REF" status = %d\n", nc, ref, status);
    return status;
}


void ownClearVisitation(vx_graph graph)
{
    vx_uint32 n = 0;
    for (n = 0; n < graph->numNodes; n++)
        graph->nodes[n]->visited = vx_false_e;
}

void ownClearExecution(vx_graph graph)
{
    vx_uint32 n = 0;
    for (n = 0; n < graph->numNodes; n++)
        graph->nodes[n]->executed = vx_false_e;
}

vx_status ownTraverseGraph(vx_graph graph,
                          vx_uint32 parentIndex,
                          vx_uint32 childIndex)
{
    /* this is expensive, but needed in order to know who references a parameter */
    static vx_uint32 refNodes[VX_INT_MAX_REF];
    /* this keeps track of the available starting point in the static buffer */
    static vx_uint32 refStart = 0;
    /* this makes sure we don't have any odd conditions about infinite depth */
    static vx_uint32 depth = 0;

    vx_uint32 refCount = 0;
    vx_uint32 refIndex = 0;
    vx_uint32 thisIndex = 0;
    vx_status status = VX_SUCCESS;
    vx_uint32 p = 0;

    VX_PRINT(VX_ZONE_GRAPH, "refStart = %u\n", refStart);

    if (parentIndex == childIndex && parentIndex != VX_INT_MAX_NODES)
    {
        VX_PRINT(VX_ZONE_ERROR, "################################\n");
        VX_PRINT(VX_ZONE_ERROR, "ERROR: CYCLE DETECTED! node[%u]\n", parentIndex);
        VX_PRINT(VX_ZONE_ERROR, "################################\n");
        /* there's a cycle in the graph */
        status = VX_ERROR_INVALID_GRAPH;
    }
    else if (depth > graph->numNodes) /* should be impossible under normal circumstances */
    {
        /* there's a cycle in the graph */
        status = VX_ERROR_INVALID_GRAPH;
    }
    else
    {
        /* if the parent is an invalid index, then we assume we're processing a
         * head of a graph which has no parent index.
         */
        if (parentIndex == VX_INT_MAX_NODES)
        {
            parentIndex = childIndex;
            thisIndex = parentIndex;
            VX_PRINT(VX_ZONE_GRAPH, "Starting head-first traverse of graph from node[%u]\n", thisIndex);
        }
        else
        {
            thisIndex = childIndex;
            VX_PRINT(VX_ZONE_GRAPH, "continuing traverse of graph from node[%u] on node[%u] start=%u\n", parentIndex, thisIndex, refStart);
        }

        for (p = 0; p < graph->nodes[thisIndex]->kernel->signature.num_parameters; p++)
        {
            vx_enum dir = graph->nodes[thisIndex]->kernel->signature.directions[p];
            vx_reference_t *ref = graph->nodes[thisIndex]->parameters[p];

            if (dir != VX_INPUT && ref != NULL)
            {
                VX_PRINT(VX_ZONE_GRAPH, "[traverse] node[%u].parameter[%u] = "VX_FMT_REF"\n", thisIndex, p, ref);
                /* send the maximum number of possible nodes to find */
                refCount = dimof(refNodes) - refStart;
                status = ownFindNodesWithReference(graph, ref, &refNodes[refStart], &refCount, VX_INPUT);
                VX_PRINT(VX_ZONE_GRAPH, "status = %d at node[%u] start=%u count=%u\n", status, thisIndex, refStart, refCount);
                if (status == VX_SUCCESS)
                {
                    vx_uint32 refStop = refStart + refCount;
                    VX_PRINT(VX_ZONE_GRAPH, "Looping from %u to %u\n", refStart, refStop);
                    for (refIndex = refStart; refIndex < refStop; refIndex++)
                    {
                        vx_status child_status = VX_SUCCESS;
                        VX_PRINT(VX_ZONE_GRAPH, "node[%u] => node[%u]\n", thisIndex, refNodes[refIndex]);
                        refStart += refCount;
                        depth++; /* go one more level in */
                        child_status = ownTraverseGraph(graph, thisIndex, refNodes[refIndex]);
                        if (child_status != VX_SUCCESS)
                            status = child_status;
                        depth--; /* pull out one level */
                        refStart -= refCount;
                        VX_PRINT(VX_ZONE_GRAPH, "status = %d at node[%u]\n", status, thisIndex);
                    }
                }
                if (status == VX_ERROR_INVALID_LINK) /* no links at all */
                {
                    VX_PRINT(VX_ZONE_GRAPH, "[Ok] No link found for node[%u].parameter[%u]\n", thisIndex, p);
                    status = VX_SUCCESS;
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_GRAPH, "[ ignore ] node[%u].parameter[%u] = "VX_FMT_REF" type %d\n", childIndex, p, ref, dir);
            }
            if (status == VX_ERROR_INVALID_GRAPH)
                break;
        }

        if (status == VX_SUCCESS)
        {
            /* mark it visited for the next check to pass */
            graph->nodes[thisIndex]->visited = vx_true_e;
        }
    }
    VX_PRINT(VX_ZONE_GRAPH, "returning status %d\n", status);
    return status;
}

void ownFindNextNodes(vx_graph graph,
                     vx_uint32 last_nodes[VX_INT_MAX_REF], vx_uint32 numLast,
                     vx_uint32 next_nodes[VX_INT_MAX_REF], vx_uint32 *numNext,
                     vx_uint32 left_nodes[VX_INT_MAX_REF], vx_uint32 *numLeft)
{
    vx_uint32 poss_next[VX_INT_MAX_REF];
    vx_uint32 i,n,p,n1,numPoss = 0;

    VX_PRINT(VX_ZONE_GRAPH, "Entering with %u left nodes\n", *numLeft);
    for (n = 0; n < *numLeft; n++)
    {
        VX_PRINT(VX_ZONE_GRAPH, "leftover: node[%u] = %s\n", left_nodes[n], graph->nodes[left_nodes[n]]->kernel->name);
    }

    numPoss = 0;
    *numNext = 0;

    /* for each last node, add all output to input nodes to the list of possible. */
    for (i = 0; i < numLast; i++)
    {
        n = last_nodes[i];
        for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
        {
            vx_enum dir = graph->nodes[n]->kernel->signature.directions[p];
            vx_reference_t *ref =  graph->nodes[n]->parameters[p];
            if (((dir == VX_OUTPUT) || (dir == VX_BIDIRECTIONAL)) && (ref != NULL))
            {
                /* send the max possible nodes */
                n1 = dimof(poss_next) - numPoss;
                if (ownFindNodesWithReference(graph, ref, &poss_next[numPoss], &n1, VX_INPUT) == VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_GRAPH, "Adding %u nodes to possible list\n", n1);
                    numPoss += n1;
                }
            }
        }
    }

    VX_PRINT(VX_ZONE_GRAPH, "There are %u possible nodes\n", numPoss);

    /* add back all the left over nodes (making sure to not include duplicates) */
    for (i = 0; i < *numLeft; i++)
    {
        vx_uint32 j;
        vx_bool match = vx_false_e;
        for (j = 0; j < numPoss; j++)
        {
            if (left_nodes[i] == poss_next[j])
            {
                match = vx_true_e;
            }
        }
        if (match == vx_false_e)
        {
            VX_PRINT(VX_ZONE_GRAPH, "Adding back left over node[%u] %s\n", left_nodes[i], graph->nodes[left_nodes[i]]->kernel->name);
            poss_next[numPoss++] = left_nodes[i];
        }
    }
    *numLeft = 0;

    /* now check all possible next nodes to see if the parent nodes are visited. */
    for (i = 0; i < numPoss; i++)
    {
        vx_uint32 poss_params[VX_INT_MAX_PARAMS];
        vx_uint32 pi, numPossParam = 0;
        vx_bool ready = vx_true_e;

        n = poss_next[i];
        VX_PRINT(VX_ZONE_GRAPH, "possible: node[%u] = %s\n", n, graph->nodes[n]->kernel->name);
        for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
        {
            if (graph->nodes[n]->kernel->signature.directions[p] == VX_INPUT)
            {
                VX_PRINT(VX_ZONE_GRAPH,"nodes[%u].parameter[%u] predicate needs to be checked\n", n, p);
                poss_params[numPossParam] = p;
                numPossParam++;
            }
        }

        /* now check to make sure all possible input parameters have their */
        /* parent nodes executed. */
        for (pi = 0; pi < numPossParam; pi++)
        {
            vx_uint32 predicate_nodes[VX_INT_MAX_REF];
            vx_uint32 predicate_count = 0;
            vx_uint32 predicate_index = 0;
            vx_uint32 refIdx = 0;
            vx_reference_t *ref = 0;
            vx_enum reftype[2] = {VX_OUTPUT, VX_BIDIRECTIONAL};

            p = poss_params[pi];
            ref = graph->nodes[n]->parameters[p];
            VX_PRINT(VX_ZONE_GRAPH, "checking node[%u].parameter[%u] = "VX_FMT_REF"\n", n, p, ref);

            for(refIdx = 0; refIdx < dimof(reftype); refIdx++) {
                /* set the size of predicate nodes going in */
                predicate_count = dimof(predicate_nodes);
                if (ownFindNodesWithReference(graph, ref, predicate_nodes, &predicate_count, reftype[refIdx]) == VX_SUCCESS)
                {
                    /* check to see of all of the predicate nodes are executed */
                    for (predicate_index = 0;
                         predicate_index < predicate_count;
                         predicate_index++)
                    {
                        n1 = predicate_nodes[predicate_index];
                        if (graph->nodes[n1]->executed == vx_false_e)
                        {
                            VX_PRINT(VX_ZONE_GRAPH, "predicated: node[%u] = %s\n", n1, graph->nodes[n1]->kernel->name);
                            ready = vx_false_e;
                            break;
                        }
                    }
                }
                if(ready == vx_false_e) {
                    break;
                }
            }
        }
        if (ready == vx_true_e)
        {
            /* make sure we don't schedule this node twice */
            if (graph->nodes[n]->visited == vx_false_e)
            {
                next_nodes[(*numNext)++] = n;
                graph->nodes[n]->visited = vx_true_e;
            }
        }
        else
        {
            /* put the node back into the possible list for next time */
            left_nodes[(*numLeft)++] = n;
            VX_PRINT(VX_ZONE_GRAPH, "notready: node[%u] = %s\n", n, graph->nodes[n]->kernel->name);
        }
    }

    VX_PRINT(VX_ZONE_GRAPH, "%u Next Nodes\n", *numNext);
    for (i = 0; i < *numNext; i++)
    {
        n = next_nodes[i];
        VX_PRINT(VX_ZONE_GRAPH, "next: node[%u] = %s\n", n, graph->nodes[n]->kernel->name);
    }
    VX_PRINT(VX_ZONE_GRAPH, "%u Left Nodes\n", *numLeft);
    for (i = 0; i < *numLeft; i++)
    {
        n = left_nodes[i];
        VX_PRINT(VX_ZONE_GRAPH, "left: node[%u] = %s\n", n, graph->nodes[n]->kernel->name);
    }
}

void ownContaminateGraphs(vx_reference ref)
{
    if (ownIsValidReference(ref) == vx_true_e)
    {
        vx_uint32 r;
        vx_context_t *context = ref->context;
        /*! \internal Scan the entire context for graphs which may contain
         * this reference and mark them as unverified.
         */
        ownSemWait(&context->base.lock);
        for (r = 0u; r < context->num_references; r++)
        {
            if (context->reftable[r] == NULL)
                continue;
            if (context->reftable[r]->type == VX_TYPE_GRAPH)
            {
                vx_uint32 n;
                vx_bool found = vx_false_e;
                vx_graph graph = (vx_graph_t *)context->reftable[r];
                for (n = 0u; n < (graph->numNodes) && (found == vx_false_e); n++)
                {
                    vx_uint32 p;
                    for (p = 0u; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
                    {
                        if (graph->nodes[n]->kernel->signature.directions[p] == VX_OUTPUT)
                        {
                            continue;
                        }
                        if (graph->nodes[n]->parameters[p] == ref)
                        {
                            found = vx_true_e;
                            graph->reverify = graph->verified;
                            graph->verified = vx_false_e;
                            graph->state = VX_GRAPH_STATE_UNVERIFIED;
                            break;
                        }
                    }
                }
            }
        }
        ownSemPost(&context->base.lock);

    }
}

/******************************************************************************/
/* PUBLIC FUNCTIONS */
/******************************************************************************/

VX_API_ENTRY vx_graph VX_API_CALL vxCreateGraph(vx_context c)
{
    vx_graph_t * graph = NULL;
    vx_context_t *context = (vx_context_t *)c;

    if (ownIsValidContext(context) == vx_true_e)
    {
        graph = (vx_graph)ownCreateReference(context, VX_TYPE_GRAPH, VX_EXTERNAL, &context->base);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS && graph->base.type == VX_TYPE_GRAPH)
        {
            ownInitPerf(&graph->perf);
            ownCreateSem(&graph->lock, 1);
            VX_PRINT(VX_ZONE_GRAPH,"Created Graph %p\n", graph);
            ownPrintReference((vx_reference_t *)graph);
            graph->reverify = graph->verified;
            graph->verified = vx_false_e;
            graph->state = VX_GRAPH_STATE_UNVERIFIED;
        }
    }

    return (vx_graph)graph;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetGraphAttribute(vx_graph graph, vx_enum attribute, const void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    (void)attribute;
    (void)ptr;
    (void)size;

    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e)
    {
        /*! \todo there are no settable attributes in this implementation yet... */
        status = VX_ERROR_NOT_SUPPORTED;
    }
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryGraph(vx_graph graph, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidReference(&graph->base) == vx_true_e)
    {

        VX_PRINT(VX_ZONE_GRAPH,"INFO: Query:0x%x:%d\n", attribute, (attribute & VX_ATTRIBUTE_ID_MASK));

        switch (attribute)
        {
            case VX_GRAPH_PERFORMANCE:
                if (VX_CHECK_PARAM(ptr, size, vx_perf_t, 0x3))
                {
                    memcpy(ptr, &graph->perf, size);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_GRAPH_STATE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_status *)ptr = graph->state;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_GRAPH_NUMNODES:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = graph->numNodes;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_GRAPH_NUMPARAMETERS:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = graph->numParams;
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

void ownDestructGraph(vx_reference ref)
{
    vx_graph graph = (vx_graph)ref;
    while (graph->numNodes)
    {
        vx_node node = (vx_node)graph->nodes[0];
        /* Interpretation of spec is to release all external references of Nodes when vxReleaseGraph()
           is called AND all graph references count == 0 (garbage collection).
           However, it may be possible that the user would have already released its external reference
           so we need to check. */
        if(node->base.external_count) {
            ownReleaseReferenceInt((vx_reference *)&node, VX_TYPE_NODE, VX_EXTERNAL, NULL);
        }
        ownRemoveNodeInt(&graph->nodes[0]);
    }
    // execution lock?
    ownDestroySem(&graph->lock);
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseGraph(vx_graph *g)
{
    return ownReleaseReferenceInt((vx_reference *)g, VX_TYPE_GRAPH, VX_EXTERNAL, NULL);
}

/* Do a topological in-place sort of the nodes in list, with current
   order maintained between independent nodes. */
static void vxTopologicalSort(vx_graph graph, vx_node *list, vx_uint32 nnodes)
{
   /* Knuth TAoCP algorithm 2.2.3 T.
      Nodes and their parameters are the "objects" which have partial-order
      relations (with "<" noting the similar-looking general symbol for
      relational order, not the specific less-than relation), and it's
      always pair-wise: node < parameter for outputs,
      parameter < node for inputs. */
    vx_uint32 nobjects;         /* Number of objects; "n" in TAoCP. */
    vx_uint32 nremain;          /* Number of remaining objects to be "output"; "N" in TAoCP. */
    vx_uint32 objectno;         /* Running count, 1-based. */
    vx_uint32 j, k, n, r, f;
    vx_uint32 outputnr;         /* Running count of nodes as they're "output". */

    struct direct_successor {
        vx_uint32 suc;
        struct direct_successor *next;
    };

    struct object_relations {
        union {
            vx_uint32 count;
            vx_uint32 qlink;
        } u;
        struct direct_successor *top;
        vx_reference ref;
    };

    struct object_relations *x;
    struct direct_successor *suc_next_table;
    struct direct_successor *avail;

    /* Visit each node in the list and its in- and out-parameters,
       clearing all indices. Find upper bound for nobjects, for use when
       allocating x (X in the algorithm). This number is also the exact
       number of relations, for use with suc_next_table (the unnamed
       "suc, next" table in the algorithm). */
    vx_uint32 max_n_objects_relations = nnodes;

    for (n = 0; n < nnodes; n++)
    {
        vx_uint32 parmno;

        max_n_objects_relations += list[n]->kernel->signature.num_parameters;

        for (parmno = 0; parmno < list[n]->kernel->signature.num_parameters; parmno++)
        {
            /* Pick the parent object in case of su-objects (e.g., ROI) */
            vx_reference ref = list[n]->parameters[parmno];
            while ( (ref != NULL) &&
                    (ref->scope != NULL) &&
                    (ref->scope != (vx_reference)graph) &&
                    (ref->scope != (vx_reference)ref->context)
                    )
            {
                ref = ref->scope;
            }

            if (ref != NULL)
            {
                ref->index = 0;
            }
            else
            {
                /* Ignore NULL (optional) parameters. */
                max_n_objects_relations--;
            }
        }
    }

    /* Visit each node and its parameters, setting all indices. Allocate
       and initialize the node + parameters-list. The x table is
       1-based; index 0 is a sentinel.
       (This is step T1.) */
    x = calloc(max_n_objects_relations + 1, sizeof (*x));
    suc_next_table = calloc(max_n_objects_relations, sizeof (*x));

    avail = suc_next_table;

    for (objectno = 1; objectno <= nnodes; objectno++)
    {
        vx_node node = list[objectno - 1];
        node->base.index = objectno;
        x[objectno].ref = (vx_reference)node;
    }

    /* While we visit the parameters (setting their index if 0), we
       "input" the relation. We don't have to iterate separately after
       all parameters are "indexed", as all nodes are already in place
       (and "indexed") and a parameter doesn't have a direct relation
       with another parameter.
       (Steps T2 and T3). */
    for (n = 0; n < nnodes; n++)
    {
        vx_uint32 parmno;

        for (parmno = 0; parmno < list[n]->kernel->signature.num_parameters; parmno++)
        {
            vx_reference ref = list[n]->parameters[parmno];
            struct direct_successor *p;

            /* Pick the parent object in case of su-objects (e.g., ROI) */
            while ( (ref != NULL) &&
                    (ref->scope != NULL) &&
                    (ref->scope != (vx_reference)graph) &&
                    (ref->scope != (vx_reference)ref->context)
                    )
            {
                ref = ref->scope;
            }

            if (ref == NULL)
            {
                continue;
            }

            if (ref->index == 0)
            {
                x[objectno].ref = ref;
                ref->index = objectno++;
            }

            /* Step T2. */
            if (list[n]->kernel->signature.directions[parmno] == VX_INPUT)
            {
                /* parameter < node */
                j = ref->index;
                k = n + 1;
            }
            else
            {
                /* node < parameter */
                k = ref->index;
                j = n + 1;
            }

            /* Step T3. */
            x[k].u.count++;
            p = avail++;
            p->suc = k;
            p->next = x[j].top;
            x[j].top = p;
        }
    }

    /* With a 1-based index, we need to back-off one to get the number of objects. */
    nobjects = objectno - 1;
    nremain = nobjects;

    /* At this point, we could visit all the nodes in
       x[1..graph->numNodes] (all the first graph->numNodes in x are
       nodes) and put those with
       count <= node->kernel->signature.num_parameters in graph->heads
       (as a node is always dependent on its input parameters and all
       nodes have inputs, but some may be NULL), avoiding the
       O(numNodes**2) loop later in our caller. */

    /* Step T4. Note that we're not zero-based but 1-based; 0 and x[0]
       are sentinels. */
    r = 0;
    x[0].u.qlink = 0;
    for (k = 1; k <= nobjects; k++)
    {
        if (x[k].u.count == 0)
        {
            x[r].u.qlink = k;
            r = k;
        }
    }

    f = x[0].u.qlink;
    outputnr = 0;

    /* Step T5. (We don't output a final 0 as present in the algorithm.) */
    while (f != 0)
    {
        struct direct_successor *p;

        /* This is our "output". Nodes only; we don't otherwise make
           use the order in which parameters are being processed. */
        if (x[f].ref->type == VX_TYPE_NODE)
            list[outputnr++] = (vx_node)x[f].ref;
        nremain--;
        p = x[f].top;

        /* Step T6. */
        while (p != NULL)
        {
            if (--x[p->suc].u.count == 0)
            {
                x[r].u.qlink = p->suc;
                r = p->suc;
            }
            p = p->next;
        }

        /* Step T7 */
        f = x[f].u.qlink;
    }

    /* Step T8.
       At this point, we could inspect nremain and if non-zero, we have
       a cycle. To wit, we can avoid the O(numNodes**2) loop later in
       our caller. We have to do check for cycles anyway, because if
       there was a cycle we need to restore *all* the nodes into list[],
       or else they can't be disposed of properly. We use the original
       order, both for simplicity and because the caller might be making
       invalid assumptions; having passed an incorrect graph is cause
       for concern about integrity. */
    if (nremain != 0)
        for (n = 0; n < nnodes; n++)
            list[n] = (vx_node)x[n+1].ref;

    free(suc_next_table);
    free(x);
}

static vx_bool setup_output(vx_graph graph, vx_uint32 n, vx_uint32 p, vx_reference_t** vref, vx_meta_format* meta,
                            vx_status* status, vx_uint32* num_errors)
{
    *vref = graph->nodes[n]->parameters[p];
    *meta = ownCreateMetaFormat(graph->base.context);

    /* check to see if the reference is virtual */
    if ((*vref)->is_virtual == vx_false_e)
    {
        *vref = NULL;
    }
    else
    {
        VX_PRINT(VX_ZONE_GRAPH, "Virtual Reference detected at kernel %S parameter %u\n",
                graph->nodes[n]->kernel->name,
                p);
        if ((*vref)->scope->type == VX_TYPE_GRAPH &&
            (*vref)->scope != (vx_reference_t *)graph &&
            /* We check only one level up; we make use of the
               knowledge that this implementation has no more
               than one level of child-graphs. (Nodes are only
               one level; no child-graph-using node is composed
               from other child-graph-using nodes.) We need
               this check (for example) for a virtual image
               being an output parameter to a node for which
               this graph is the child-graph implementation,
               like in vx_bug13517.c. */
            (*vref)->scope != (vx_reference_t *)graph->parentGraph)
        {
            /* major fault! */
            *status = VX_ERROR_INVALID_SCOPE;
            vxAddLogEntry((vx_reference)(*vref), *status, "Virtual Reference is in the wrong scope, created from another graph!\n");
            (*num_errors)++;
            return vx_false_e; //break;
        }
        /* ok if context, pyramid or this graph */
    }

    /* the type of the parameter is known by the system, so let the system set it by default. */
    (*meta)->type = graph->nodes[n]->kernel->signature.types[p];

    return vx_true_e;
}

/*
// Parameters:
//   n    - index of node
//   p    - index of parameter
//   vref - reference of the parameter
//   meta - parameter meta info
*/
static vx_bool postprocess_output_data_type(vx_graph graph, vx_uint32 n, vx_uint32 p, vx_reference_t* item, vx_reference_t* vref, vx_meta_format meta,
                                  vx_status* status, vx_uint32* num_errors)
{
    if (ownIsValidType(meta->type) == vx_false_e)
    {
        *status = VX_ERROR_INVALID_TYPE;
        vxAddLogEntry(&graph->base, *status,
            "Node: %s: parameter[%u] is not a valid type %d!\n",
            graph->nodes[n]->kernel->name, p, meta->type);
        (*num_errors)++;
        return vx_false_e; // exit on error
    }

    if (meta->type == VX_TYPE_IMAGE)
    {
        vx_image img = (vx_image)item;
        VX_PRINT(VX_ZONE_GRAPH, "meta: type 0x%08x, %ux%u\n", meta->type, meta->dim.image.width, meta->dim.image.height);
        if (vref == (vx_reference_t*)img)
        {
            VX_PRINT(VX_ZONE_GRAPH, "Creating Image From Meta Data!\n");
            /*! \todo need to worry about images that have a format, but no dimensions too */
            if (img->format == VX_DF_IMAGE_VIRT || img->format == meta->dim.image.format)
            {
                img->format = meta->dim.image.format;
                img->width = meta->dim.image.width;
                img->height = meta->dim.image.height;
                /* we have to go set all the other dimensional information up. */
                ownInitImage(img, img->width, img->height, img->format);
                ownPrintImage(img); /* show that it's been created. */
            }
            else
            {
                *status = VX_ERROR_INVALID_FORMAT;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] has invalid format %08x!\n",
                    graph->nodes[n]->kernel->name, p, img->format);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has invalid format %08x!\n",
                    graph->nodes[n]->kernel->name, p, img->format);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
        }
        else
        {
            /* check the data that came back from the output validator against the object */
            if ((img->width != meta->dim.image.width) ||
                (img->height != meta->dim.image.height))
            {
                *status = VX_ERROR_INVALID_DIMENSION;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] is an invalid dimension %ux%u!\n",
                    graph->nodes[n]->kernel->name, p, img->width, img->height);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] is an invalid dimension %ux%u!\n",
                    graph->nodes[n]->kernel->name, p, img->width, img->height);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
            if (img->format != meta->dim.image.format)
            {
                *status = VX_ERROR_INVALID_FORMAT;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] is an invalid format %08x!\n",
                    graph->nodes[n]->kernel->name, p, img->format);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has invalid format %08x!\n",
                    graph->nodes[n]->kernel->name, p, img->format);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
        }

        if (NULL != meta->set_valid_rectangle_callback)
            graph->nodes[n]->attributes.valid_rect_reset = vx_false_e;

        if (vx_false_e == graph->nodes[n]->attributes.valid_rect_reset &&
            NULL != meta->set_valid_rectangle_callback)
        {
            /* calculate image valid rectangle through callback */

            vx_uint32 i;
            vx_uint32 nparams = 0;
            vx_uint32 num_in_images = 0;
            vx_rectangle_t** in_rect = NULL;
            vx_rectangle_t* out_rect[1] = { NULL };
            vx_node node = graph->nodes[n];

            /* assume no errors */
            vx_bool res = vx_true_e;

            if (VX_SUCCESS != vxQueryNode(node, VX_NODE_PARAMETERS, &nparams, sizeof(nparams)))
            {
                *status = VX_FAILURE;
                return vx_false_e;
            }

            /* compute num of input images */
            for (i = 0; i < nparams; i++)
            {
                if (VX_INPUT == node->kernel->signature.directions[i] &&
                    VX_TYPE_IMAGE == node->parameters[i]->type)
                {
                    num_in_images++;
                }
            }

            /* allocate array of pointers to input images valid rectangles */
            in_rect = (vx_rectangle_t**)malloc(num_in_images * sizeof(vx_rectangle_t*));
            if (NULL == in_rect)
            {
                *status = VX_FAILURE;
                return vx_false_e;
            }

            for (i = 0; i < nparams; i++)
            {
                if (VX_INPUT == node->kernel->signature.directions[i] &&
                    VX_TYPE_IMAGE == node->parameters[i]->type)
                {
                    in_rect[i] = (vx_rectangle_t*)malloc(sizeof(vx_rectangle_t));
                    if (NULL == in_rect[i])
                    {
                        *status = VX_FAILURE;
                        res = vx_false_e;
                        break;
                    }

                    /* collect input images valid rectagles in array */
                    if (VX_SUCCESS != vxGetValidRegionImage((vx_image)node->parameters[i], in_rect[i]))
                    {
                        *status = VX_FAILURE;
                        res = vx_false_e;
                        break;
                    }
                }
            }

            if (vx_false_e != res)
            {
                out_rect[0] = (vx_rectangle_t*)malloc(sizeof(vx_rectangle_t));
                if (NULL == out_rect[0])
                {
                    *status = VX_FAILURE;
                    res = vx_false_e;
                }

                /* calculate output image valid rectangle */
                if (VX_SUCCESS == meta->set_valid_rectangle_callback(graph->nodes[n], p, (const vx_rectangle_t* const*)in_rect,
                                                                     (vx_rectangle_t* const*)out_rect))
                {
                    /* set output image valid rectangle */
                    if (VX_SUCCESS != vxSetImageValidRectangle(img, (const vx_rectangle_t*)out_rect[0]))
                    {
                        *status = VX_FAILURE;
                        res = vx_false_e;
                    }
                }
                else
                {
                    *status = VX_FAILURE;
                    res = vx_false_e;
                }
            }

            /* deallocate arrays memory */
            for (i = 0; i < num_in_images; i++)
            {
                if (NULL != in_rect[i])
                {
                    free(in_rect[i]);
                }
            }

            if (NULL != in_rect)
                free(in_rect);
            if (NULL != out_rect[0])
                free(out_rect[0]);


            return res;
        }

        if (vx_true_e == graph->nodes[n]->attributes.valid_rect_reset)
        {
            /* reset image valid rectangle */
            vx_rectangle_t out_rect;
            out_rect.start_x = 0;
            out_rect.start_y = 0;
            out_rect.end_x   = img->width;
            out_rect.end_y   = img->height;

            if (VX_SUCCESS != vxSetImageValidRectangle(img, &out_rect))
            {
                *status = VX_FAILURE;
                return vx_false_e;
            }
        }
    } /* VX_TYPE_IMAGE */
    else if (meta->type == VX_TYPE_ARRAY)
    {
        vx_array_t *arr = (vx_array_t *)item;
        VX_PRINT(VX_ZONE_GRAPH, "meta: type 0x%08x, 0x%08x "VX_FMT_SIZE"\n", meta->type, meta->dim.array.item_type, meta->dim.array.capacity);
        if (vref == (vx_reference_t *)arr)
        {
            VX_PRINT(VX_ZONE_GRAPH, "Creating Array From Meta Data %x and "VX_FMT_SIZE"!\n", meta->dim.array.item_type, meta->dim.array.capacity);
            if (ownInitVirtualArray(arr, meta->dim.array.item_type, meta->dim.array.capacity) != vx_true_e)
            {
                *status = VX_ERROR_INVALID_DIMENSION;
                vxAddLogEntry(&graph->base, VX_ERROR_INVALID_DIMENSION,
                    "Node: %s: meta[%u] has an invalid item type 0x%08x or capacity "VX_FMT_SIZE"\n",
                    graph->nodes[n]->kernel->name, p, meta->dim.array.item_type, meta->dim.array.capacity);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: meta[%u] has an invalid item type 0x%08x or capacity "VX_FMT_SIZE"\n",
                    graph->nodes[n]->kernel->name, p, meta->dim.array.item_type, meta->dim.array.capacity);
                (*num_errors)++;
                return vx_false_e; //break;
            }
        }
        else
        {
            if (ownValidateArray(arr, meta->dim.array.item_type, meta->dim.array.capacity) != vx_true_e)
            {
                *status = VX_ERROR_INVALID_DIMENSION;
                vxAddLogEntry(&graph->base, VX_ERROR_INVALID_DIMENSION,
                    "Node: %s: parameter[%u] has an invalid item type 0x%08x or capacity "VX_FMT_SIZE"\n",
                    graph->nodes[n]->kernel->name, p, arr->item_type, arr->capacity);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has an invalid item type 0x%08x or capacity "VX_FMT_SIZE"\n",
                    graph->nodes[n]->kernel->name, p, arr->item_type, arr->capacity);
                (*num_errors)++;
                return vx_false_e; //break;
            }
        }
    }
    else if (meta->type == VX_TYPE_PYRAMID)
    {
        vx_pyramid_t *pyramid = (vx_pyramid_t *)item;

        vx_uint32 i;
        vx_bool res = vx_true_e;

        VX_PRINT(VX_ZONE_GRAPH, "meta: type 0x%08x, %ux%u:%u:%lf\n",
            meta->type,
            meta->dim.pyramid.width,
            meta->dim.pyramid.height,
            meta->dim.pyramid.levels,
            meta->dim.pyramid.scale);
        VX_PRINT(VX_ZONE_GRAPH, "Nodes[%u] %s parameters[%u]\n", n, graph->nodes[n]->kernel->name, p);

        if ((pyramid->numLevels != meta->dim.pyramid.levels) ||
            (pyramid->scale != meta->dim.pyramid.scale))
        {
            *status = VX_ERROR_INVALID_VALUE;
            vxAddLogEntry(&graph->base, *status, "Either levels (%u?=%u) or scale (%lf?=%lf) are invalid\n",
                pyramid->numLevels, meta->dim.pyramid.levels,
                pyramid->scale, meta->dim.pyramid.scale);
            (*num_errors)++;
            return vx_false_e; //break;
        }

        if ((pyramid->format != VX_DF_IMAGE_VIRT) &&
            (pyramid->format != meta->dim.pyramid.format))
        {
            *status = VX_ERROR_INVALID_FORMAT;
            vxAddLogEntry(&graph->base, *status, "Invalid pyramid format %x, needs %x\n",
                pyramid->format,
                meta->dim.pyramid.format);
            (*num_errors)++;
            return vx_false_e; //break;
        }

        if (((pyramid->width != 0) &&
            (pyramid->width != meta->dim.pyramid.width)) ||
            ((pyramid->height != 0) &&
            (pyramid->height != meta->dim.pyramid.height)))
        {
            *status = VX_ERROR_INVALID_DIMENSION;
            vxAddLogEntry(&graph->base, *status, "Invalid pyramid dimensions %ux%u, needs %ux%u\n",
                pyramid->width, pyramid->height,
                meta->dim.pyramid.width, meta->dim.pyramid.height);
            (*num_errors)++;
            return vx_false_e; //break;
        }

        /* check to see if the pyramid is virtual */
        if (vref == (vx_reference_t *)pyramid)
        {
            ownInitPyramid(pyramid,
                meta->dim.pyramid.levels,
                meta->dim.pyramid.scale,
                meta->dim.pyramid.width,
                meta->dim.pyramid.height,
                meta->dim.pyramid.format);
        }

        if (NULL != meta->set_valid_rectangle_callback)
            graph->nodes[n]->attributes.valid_rect_reset = vx_false_e;

        if (vx_false_e == graph->nodes[n]->attributes.valid_rect_reset &&
            NULL != meta->set_valid_rectangle_callback)
        {
            /* calculate pyramid levels valid rectangles */

            vx_uint32 nparams = 0;
            vx_uint32 num_in_images = 0;
            vx_rectangle_t** in_rect = NULL;
            vx_rectangle_t** out_rect = NULL;

            vx_node node = graph->nodes[n];

            if (VX_SUCCESS != vxQueryNode(node, VX_NODE_PARAMETERS, &nparams, sizeof(nparams)))
            {
                *status = VX_FAILURE;
                return vx_false_e;
            }

            /* compute num of input images */
            for (i = 0; i < nparams; i++)
            {
                if (VX_INPUT == node->kernel->signature.directions[i] &&
                    VX_TYPE_IMAGE == node->parameters[i]->type)
                {
                    num_in_images++;
                }
            }

            in_rect = (vx_rectangle_t**)malloc(num_in_images * sizeof(vx_rectangle_t*));
            if (NULL == in_rect)
            {
                *status = VX_FAILURE;
                return vx_false_e;
            }

            for (i = 0; i < num_in_images; i++)
                in_rect[i] = NULL;

            for (i = 0; i < nparams; i++)
            {
                if (VX_INPUT == node->kernel->signature.directions[i] &&
                    VX_TYPE_IMAGE == node->parameters[i]->type)
                {
                    in_rect[i] = (vx_rectangle_t*)malloc(sizeof(vx_rectangle_t));
                    if (NULL == in_rect[i])
                    {
                        *status = VX_FAILURE;
                        res = vx_false_e;
                        break;
                    }

                    if (VX_SUCCESS != vxGetValidRegionImage((vx_image)node->parameters[i], in_rect[i]))
                    {
                        *status = VX_FAILURE;
                        res = vx_false_e;
                        break;
                    }
                }
            }

            if (vx_false_e != res)
            {
                out_rect = (vx_rectangle_t**)malloc(meta->dim.pyramid.levels * sizeof(vx_rectangle_t*));
                if (NULL != out_rect)
                {
                    vx_uint32 k;
                    for (k = 0; k < meta->dim.pyramid.levels; k++)
                        out_rect[k] = NULL;

                    for (i = 0; i < meta->dim.pyramid.levels; i++)
                    {
                        out_rect[i] = (vx_rectangle_t*)malloc(sizeof(vx_rectangle_t));
                        if (NULL == out_rect[i])
                        {
                            *status = VX_FAILURE;
                            res = vx_false_e;
                            break;
                        }
                    }
                }
                else
                {
                    *status = VX_FAILURE;
                    res = vx_false_e;
                }
            }

            if (vx_false_e != res)
            {
                /* calculate pyramid levels valid rectangles */
                if (VX_SUCCESS == meta->set_valid_rectangle_callback(graph->nodes[n], p, (const vx_rectangle_t* const*)in_rect, out_rect))
                {
                    for (i = 0; i < meta->dim.pyramid.levels; i++)
                    {
                        vx_image img = vxGetPyramidLevel(pyramid, i);

                        if (vx_false_e == ownIsValidSpecificReference((vx_reference)img, VX_TYPE_IMAGE))
                        {
                            *status = VX_FAILURE;
                            res = vx_false_e;
                            vxReleaseImage(&img); /* already on error path, ignore additional errors */
                            break;
                        }

                        if (VX_SUCCESS != vxSetImageValidRectangle(img, out_rect[i]))
                        {
                            *status = VX_FAILURE;
                            res = vx_false_e;
                            vxReleaseImage(&img); /* already on error path, ignore additional errors */
                            break;
                        }

                        if (VX_SUCCESS != vxReleaseImage(&img))
                        {
                            *status = VX_FAILURE;
                            res = vx_false_e;
                            break;
                        }
                    } /* for pyramid levels */
                }
                else
                {
                    *status = VX_FAILURE;
                    res = vx_false_e;
                }
            } /* if successful memory allocation */

            /* deallocate rectangle arrays */
            for (i = 0; i < num_in_images; i++)
            {
                if (NULL != in_rect && NULL != in_rect[i])
                {
                    free(in_rect[i]);
                }
            }

            if (NULL != in_rect)
                free(in_rect);

            for (i = 0; i < meta->dim.pyramid.levels; i++)
            {
                if (NULL != out_rect && NULL != out_rect[i])
                    free(out_rect[i]);
            }

            if (NULL != out_rect)
                free(out_rect);

            return res;
        }

        if (vx_true_e == graph->nodes[n]->attributes.valid_rect_reset)
        {
            /* reset output pyramid levels valid rectangles */

            vx_bool res = vx_true_e;

            for (i = 0; i < meta->dim.pyramid.levels; i++)
            {
                vx_uint32 width = 0;
                vx_uint32 height = 0;
                vx_rectangle_t out_rect;

                vx_image img = vxGetPyramidLevel(pyramid, i);

                if (vx_false_e == ownIsValidSpecificReference((vx_reference)img, VX_TYPE_IMAGE))
                {
                    *status = VX_FAILURE;
                    return vx_false_e;
                }

                if (VX_SUCCESS != vxQueryImage(img, VX_IMAGE_WIDTH, &width, sizeof(width)))
                {
                    *status = VX_FAILURE;
                    res = vx_false_e;
                    vxReleaseImage(&img); /* already on error path, ignore additional errors */
                    break;
                }

                if (VX_SUCCESS != vxQueryImage(img, VX_IMAGE_HEIGHT, &height, sizeof(height)))
                {
                    *status = VX_FAILURE;
                    res = vx_false_e;
                    vxReleaseImage(&img); /* already on error path, ignore additional errors */
                    break;
                }

                if (vx_false_e != res)
                {
                    out_rect.start_x = 0;
                    out_rect.start_y = 0;
                    out_rect.end_x   = width;
                    out_rect.end_y   = height;

                    /* pyramid level valid rectangle is a whole image */
                    if (VX_SUCCESS != vxSetImageValidRectangle(img, &out_rect))
                    {
                        *status = VX_FAILURE;
                        res = vx_false_e;
                    }
                }

                if (VX_SUCCESS != vxReleaseImage(&img))
                {
                    *status = VX_FAILURE;
                    res = vx_false_e;
                }
            } /* for pyramid levels */

            return res;
        }
    } /* VX_TYPE_PYRAMID */
    else if (meta->type == VX_TYPE_SCALAR)
    {
        vx_scalar_t *scalar = (vx_scalar_t *)item;
        if (scalar->data_type != meta->dim.scalar.type)
        {
            *status = VX_ERROR_INVALID_TYPE;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_TYPE,
                "Scalar contains invalid typed objects for node %s\n", graph->nodes[n]->kernel->name);
            (*num_errors)++;
            return vx_false_e; //break;
        }
    }
    else if (meta->type == VX_TYPE_MATRIX)
    {
        vx_matrix_t *matrix = (vx_matrix_t *)item;
        if (matrix->data_type != meta->dim.matrix.type)
        {
            *status = VX_ERROR_INVALID_TYPE;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_TYPE,
                "Node: %s: parameter[%u] has an invalid data type 0x%08x\n",
                graph->nodes[n]->kernel->name, p, matrix->data_type);
            (*num_errors)++;
            return vx_false_e; //break;
        }

        if (matrix->columns != meta->dim.matrix.cols || matrix->rows != meta->dim.matrix.rows)
        {
            *status = VX_ERROR_INVALID_DIMENSION;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_DIMENSION,
                "Node: %s: parameter[%u] has an invalid matrix dimention %ux%u\n",
                graph->nodes[n]->kernel->name, p, matrix->data_type, matrix->rows, matrix->columns);
            (*num_errors)++;
            return vx_false_e; //break;
        }
    }
    else if (meta->type == VX_TYPE_DISTRIBUTION)
    {
        vx_distribution_t *distribution = (vx_distribution_t *)item;
        //fix
        if (distribution->offset_x != meta->dim.distribution.offset ||
            distribution->range_x != meta->dim.distribution.range ||
            distribution->memory.dims[0][VX_DIM_X] != meta->dim.distribution.bins)
        {
            *status = VX_ERROR_INVALID_VALUE;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_VALUE,
                "Node: %s: parameter[%u] has an invalid offset %u, number of bins %u or range %u\n",
                graph->nodes[n]->kernel->name, p, distribution->offset_x,
                distribution->memory.dims[0][VX_DIM_X], distribution->range_x);
            (*num_errors)++;
            return vx_false_e; //break;
        }
    }
    else if (meta->type == VX_TYPE_REMAP)
    {
        vx_remap_t *remap = (vx_remap_t *)item;
        if (remap->src_width != meta->dim.remap.src_width || remap->src_height != meta->dim.remap.src_height)
        {
            *status = VX_ERROR_INVALID_DIMENSION;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_DIMENSION,
                "Node: %s: parameter[%u] has an invalid source dimention %ux%u\n",
                graph->nodes[n]->kernel->name, p);
            (*num_errors)++;
            return vx_false_e; //break;
        }

        if (remap->dst_width != meta->dim.remap.dst_width || remap->dst_height != meta->dim.remap.dst_height)
        {
            *status = VX_ERROR_INVALID_DIMENSION;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_DIMENSION,
                "Node: %s: parameter[%u] has an invalid destination dimention %ux%u",
                graph->nodes[n]->kernel->name, p);
            (*num_errors)++;
            return vx_false_e; //break;
        }
    }
    else if (meta->type == VX_TYPE_LUT)
    {
        vx_lut_t *lut = (vx_lut_t *)item;
        if (lut->item_type != meta->dim.lut.type || lut->num_items != meta->dim.lut.count)
        {
            *status = VX_ERROR_INVALID_DIMENSION;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_DIMENSION,
                "Node: %s: parameter[%u] has an invalid item type 0x%08x or count "VX_FMT_SIZE"\n",
                graph->nodes[n]->kernel->name, p, lut->item_type, lut->num_items);
            (*num_errors)++;
            return vx_false_e; //break;
        }
    }
    else if (meta->type == VX_TYPE_THRESHOLD)
    {
        vx_threshold_t *threshold = (vx_threshold_t *)item;
        if (threshold->thresh_type != meta->dim.threshold.type)
        {
            *status = VX_ERROR_INVALID_TYPE;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_TYPE,
                "Threshold contains invalid typed objects for node %s\n", graph->nodes[n]->kernel->name);
            (*num_errors)++;
            return vx_false_e; //break;
        }
    }
    if (meta->type == VX_TYPE_TENSOR)
    {
        vx_tensor tensor = (vx_tensor)item;
        if (vref == (vx_reference_t*)tensor)
        {
            VX_PRINT(VX_ZONE_GRAPH, "Creating Tensor From Meta Data!\n");
            if ((tensor->data_type != VX_TYPE_INVALID) &&
                    (tensor->data_type != meta->dim.tensor.data_type || tensor->fixed_point_position != meta->dim.tensor.fixed_point_position))
            {
                *status = VX_ERROR_INVALID_FORMAT;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] has invalid data type %08x or fixed point position %d!\n",
                    graph->nodes[n]->kernel->name, p, tensor->data_type, tensor->fixed_point_position);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has invalid data type %08x or fixed point position %d!\n",
                    graph->nodes[n]->kernel->name, p, tensor->data_type, tensor->fixed_point_position);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
            if (tensor->number_of_dimensions != 0)
            {
               for (unsigned i = 0; i < tensor->number_of_dimensions; i++)
               {
                   if (tensor->dimensions[i] != 0 && tensor->dimensions[i] != meta->dim.tensor.dimensions[i])
                   {
                       *status = VX_ERROR_INVALID_DIMENSION;
                       vxAddLogEntry(&graph->base, *status,
                           "Node: %s: parameter[%u] has invalid dimension size %d in dimension %d!\n",
                           graph->nodes[n]->kernel->name, p, tensor->dimensions[i], i);
                       VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has invalid dimension size %d in dimension %d!\n",
                           graph->nodes[n]->kernel->name, p, tensor->dimensions[i], i);
                       (*num_errors)++;
                       return vx_false_e; // exit on error
                   }
               }
            }
            else if (tensor->number_of_dimensions != meta->dim.tensor.number_of_dimensions)
            {
                *status = VX_ERROR_INVALID_DIMENSION;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] has invalid dimension  %d!\n",
                    graph->nodes[n]->kernel->name, p, tensor->number_of_dimensions);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has invalid dimension %d!\n",
                    graph->nodes[n]->kernel->name, p, tensor->number_of_dimensions);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
            ownInitTensor(tensor, meta->dim.tensor.dimensions, meta->dim.tensor.number_of_dimensions, meta->dim.tensor.data_type, meta->dim.tensor.fixed_point_position);
            ownAllocateTensorMemory(tensor);
        }
        else
        {
            if (tensor->number_of_dimensions != meta->dim.tensor.number_of_dimensions)
            {
                *status = VX_ERROR_INVALID_DIMENSION;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] is an invalid number of dimensions %u!\n",
                    graph->nodes[n]->kernel->name, p, tensor->number_of_dimensions);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] is an invalid number of dimensions %u!\n",
                    graph->nodes[n]->kernel->name, p, tensor->number_of_dimensions);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
            for (unsigned i = 0; i < tensor->number_of_dimensions; i++)
            {
                if (tensor->dimensions[i] != meta->dim.tensor.dimensions[i])
                {
                    *status = VX_ERROR_INVALID_DIMENSION;
                    vxAddLogEntry(&graph->base, *status,
                        "Node: %s: parameter[%u] has an invalid dimension %u!\n",
                        graph->nodes[n]->kernel->name, p, tensor->dimensions[i]);
                    VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has an invalid dimension %u!\n",
                        graph->nodes[n]->kernel->name, p, tensor->dimensions[i]);
                    (*num_errors)++;
                    return vx_false_e; // exit on error
                }
            }
            if (tensor->data_type != meta->dim.tensor.data_type)
            {
                *status = VX_ERROR_INVALID_FORMAT;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] is an invalid data type %08x!\n",
                    graph->nodes[n]->kernel->name, p, tensor->data_type);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has invalid data type %08x!\n",
                    graph->nodes[n]->kernel->name, p, tensor->data_type);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
            if (tensor->fixed_point_position != meta->dim.tensor.fixed_point_position)
            {
                *status = VX_ERROR_INVALID_FORMAT;
                vxAddLogEntry(&graph->base, *status,
                    "Node: %s: parameter[%u] has an invalid fixed point position %08x!\n",
                    graph->nodes[n]->kernel->name, p, tensor->fixed_point_position);
                VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has invalid fixed point position  %08x!\n",
                    graph->nodes[n]->kernel->name, p, tensor->fixed_point_position);
                (*num_errors)++;
                return vx_false_e; // exit on error
            }
        }
    } /* VX_TYPE_TENSOR */
    /*! \todo support other output types for safety checks in graph verification parameters phase */
    else
    {
        VX_PRINT(VX_ZONE_GRAPH, "Returned Meta type %d\n", meta->type);
    }

    return vx_true_e;
} /* postprocess_output_data_type() */

/*
// Parameters:
//   n    - index of node
//   p    - index of parameter
//   vref - reference of the parameter
//   meta - parameter meta info
*/
static vx_bool postprocess_output(vx_graph graph, vx_uint32 n, vx_uint32 p, vx_reference_t* vref, vx_meta_format meta,
                                  vx_status* status, vx_uint32* num_errors)
{
    if (ownIsValidType(meta->type) == vx_false_e)
    {
        *status = VX_ERROR_INVALID_TYPE;
        vxAddLogEntry(&graph->base, *status,
            "Node: %s: parameter[%u] is not a valid type %d!\n",
            graph->nodes[n]->kernel->name, p, meta->type);
        (*num_errors)++;
        return vx_false_e; // exit on error
    }

    if (meta->type == VX_TYPE_OBJECT_ARRAY)
    {
        vx_object_array_t *objarr = (vx_object_array_t *)graph->nodes[n]->parameters[p];
        VX_PRINT(VX_ZONE_GRAPH, "meta: type 0x%08x, 0x%08x "VX_FMT_SIZE"\n", meta->type, meta->dim.object_array.item_type, meta->dim.object_array.num_items);

        if (ownValidateObjectArray(objarr, meta->dim.object_array.item_type, meta->dim.object_array.num_items) != vx_true_e)
        {
            *status = VX_ERROR_INVALID_DIMENSION;
            vxAddLogEntry(&graph->base, VX_ERROR_INVALID_DIMENSION,
                "Node: %s: parameter[%u] has an invalid item type 0x%08x or num_items "VX_FMT_SIZE"\n",
                graph->nodes[n]->kernel->name, p, objarr->item_type, objarr->num_items);
            VX_PRINT(VX_ZONE_ERROR, "Node: %s: parameter[%u] has an invalid item type 0x%08x or num_items "VX_FMT_SIZE"\n",
                graph->nodes[n]->kernel->name, p, objarr->item_type, objarr->num_items);
            (*num_errors)++;
            return vx_false_e; //break;
        }

        if (vref == (vx_reference_t *)objarr)
        {
            VX_PRINT(VX_ZONE_GRAPH, "Creating Object Array From Meta Data %x and "VX_FMT_SIZE"!\n", meta->dim.object_array.item_type, meta->dim.object_array.num_items);
            for (vx_uint32 i = 0; i < meta->dim.object_array.num_items; i++)
            {
                vx_reference item = vxGetObjectArrayItem(objarr, i);

                if (!postprocess_output_data_type(graph, n, p, item, vref, meta, status, num_errors))
                {
                    vxReleaseReference(&item);
                    *status = VX_ERROR_INVALID_PARAMETERS;
                    vxAddLogEntry(&graph->base, VX_ERROR_INVALID_PARAMETERS,
                        "Node: %s: meta[%u] has an invalid meta of exemplar\n",
                        graph->nodes[n]->kernel->name, p);
                    VX_PRINT(VX_ZONE_ERROR, "Node: %s: meta[%u] has an invalid meta of exemplar\n",
                        graph->nodes[n]->kernel->name, p);
                    (*num_errors)++;

                    return vx_false_e; //break;
                }

                vxReleaseReference(&item);
            }
        }
        else
        {
            /* check the data that came back from the output validator against the object */
            for (vx_uint32 i = 0; i < meta->dim.object_array.num_items; i++)
            {
                vx_reference item = vxGetObjectArrayItem(objarr, i);
                vx_reference itemref = vxGetObjectArrayItem((vx_object_array)vref, i);

                if (!postprocess_output_data_type(graph, n, p, item, itemref, meta, status, num_errors))
                {
                    vxReleaseReference(&item);
                    *status = VX_ERROR_INVALID_PARAMETERS;
                    vxAddLogEntry(&graph->base, VX_ERROR_INVALID_PARAMETERS,
                        "Node: %s: meta[%u] has an invalid meta of exemplar\n",
                        graph->nodes[n]->kernel->name, p);
                    VX_PRINT(VX_ZONE_ERROR, "Node: %s: meta[%u] has an invalid meta of exemplar\n",
                        graph->nodes[n]->kernel->name, p);
                    (*num_errors)++;

                    return vx_false_e; //break;
                }

                vxReleaseReference(&item);
            }
        }
    }
    else
    {
        return postprocess_output_data_type(graph, n, p, graph->nodes[n]->parameters[p], vref, meta, status, num_errors);
    }

    return vx_true_e;
} /* postprocess_output() */

VX_API_ENTRY vx_status VX_API_CALL vxVerifyGraph(vx_graph graph)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 num_errors = 0u;
    vx_bool first_time_verify = ((graph->verified == vx_false_e) && (graph->reverify == vx_false_e)) ? vx_true_e : vx_false_e;

    graph->verified = vx_false_e;

    if (ownIsValidReference(&graph->base) == vx_true_e)
    {
        vx_uint32 h,n,p;
        vx_bool hasACycle = vx_false_e;

        /* lock the graph */
        ownSemWait(&graph->base.lock);

       /* To properly deal with parameter dependence in the graph, the
          nodes have to be in topological order when their parameters
          are inspected and their dependent attributes -such as geometry
          and type- are propagated. */
        VX_PRINT(VX_ZONE_GRAPH,"###########################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Topological Sort Phase\n");
        VX_PRINT(VX_ZONE_GRAPH,"###########################\n");
        vxTopologicalSort(graph, graph->nodes, graph->numNodes);

        VX_PRINT(VX_ZONE_GRAPH,"###########################\n");
        VX_PRINT(VX_ZONE_GRAPH,"User Kernel Preprocess Phase! (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"###########################\n");

        for (n = 0; n <graph->numNodes; n++)
        {
            vx_node_t *node = graph->nodes[n];
            if (node->kernel->user_kernel)
            {
                if (!first_time_verify) // re-verify
                {
                    if (node->kernel->deinitialize)
                    {
                        vx_status status;
                        if (node->local_data_set_by_implementation == vx_false_e)
                            node->local_data_change_is_enabled = vx_true_e;
                        status = node->kernel->deinitialize((vx_node)node,
                                                            (vx_reference *)node->parameters,
                                                            node->kernel->signature.num_parameters);
                        node->local_data_change_is_enabled = vx_false_e;
                        if (status != VX_SUCCESS)
                        {
                            VX_PRINT(VX_ZONE_ERROR,"Failed to de-initialize kernel %s!\n", node->kernel->name);
                            goto exit;
                        }
                    }

                    if (node->kernel->attributes.localDataSize == 0)
                    {
                        if (node->attributes.localDataPtr)
                        {
                            if (!first_time_verify && node->attributes.localDataPtr)
                                free(node->attributes.localDataPtr);
                            node->attributes.localDataSize = 0;
                            node->attributes.localDataPtr = NULL;
                        }
                    }
                    node->local_data_set_by_implementation = vx_false_e;
                }
            }
        }

        VX_PRINT(VX_ZONE_GRAPH,"###########################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Parameter Validation Phase! (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"###########################\n");

        for (n = 0; n <graph->numNodes; n++)
        {
            /* check to make sure that a node has all required parameters */
            for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
            {
                if (graph->nodes[n]->kernel->signature.states[p] == VX_PARAMETER_STATE_REQUIRED)
                {
                    if (graph->nodes[n]->parameters[p] == NULL)
                    {
                        vxAddLogEntry(&graph->base, VX_ERROR_INVALID_PARAMETERS, "Node %s: Some parameters were not supplied!\n", graph->nodes[n]->kernel->name);
                        VX_PRINT(VX_ZONE_ERROR, "Node "VX_FMT_REF" (%s) Parameter[%u] was required and not supplied!\n",
                            graph->nodes[n],
                            graph->nodes[n]->kernel->name,p);
                        status = VX_ERROR_NOT_SUFFICIENT;
                        num_errors++;
                    }
                    else if (graph->nodes[n]->parameters[p]->internal_count == 0)
                    {
                        VX_PRINT(VX_ZONE_ERROR, "Internal reference counts are wrong!\n");
                        DEBUG_BREAK();
                        num_errors++;
                    }
                }
            }
            if (status != VX_SUCCESS)
            {
                goto exit;
            }

            /* debugging, show that we can detect "constant" data or "unreferenced data" */
            for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
            {
                vx_reference_t *ref = (vx_reference_t *)graph->nodes[n]->parameters[p];
                if (ref)
                {
                    if (ref->external_count == 0)
                    {
                        VX_PRINT(VX_ZONE_INFO, "%s[%u] = "VX_FMT_REF" (CONSTANT) type:%08x\n", graph->nodes[n]->kernel->name, p, ref, ref->type);
                    }
                    else
                    {
                        VX_PRINT(VX_ZONE_INFO, "%s[%u] = "VX_FMT_REF" (MUTABLE) type:%08x count:%d\n", graph->nodes[n]->kernel->name, p, ref, ref->type, ref->external_count);
                    }
                }
            }

            /* check if new style validators are provided (see bug14654) */
            if (graph->nodes[n]->kernel->validate != NULL)
            {
                vx_status validation_status = VX_SUCCESS;
                vx_reference vref[VX_INT_MAX_PARAMS];
                vx_meta_format metas[VX_INT_MAX_PARAMS];

                for (p = 0; p < dimof(metas); p++)
                {
                    metas[p] = NULL;
                    vref[p] = NULL;
                }

                for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
                {
                    if ((graph->nodes[n]->parameters[p] != NULL) &&
                        (graph->nodes[n]->kernel->signature.directions[p] == VX_OUTPUT))
                    {
                        if (setup_output(graph, n, p, &vref[p], &metas[p], &status, &num_errors) == vx_false_e)
                            break;
                    }
                }

                if (status == VX_SUCCESS)
                {
                    validation_status = graph->nodes[n]->kernel->validate((vx_node)graph->nodes[n],
                                                                          graph->nodes[n]->parameters,
                                                                          graph->nodes[n]->kernel->signature.num_parameters,
                                                                          metas);
                    if (validation_status != VX_SUCCESS)
                    {
                        status = validation_status;
                        vxAddLogEntry(&graph->base, status, "Node[%u] %s: parameter(s) failed validation!\n",
                                      n, graph->nodes[n]->kernel->name);
                        VX_PRINT(VX_ZONE_GRAPH,"Failed on validation of parameter(s) of kernel %s in node #%d (status=%d)\n",
                                 graph->nodes[n]->kernel->name, n, status);
                        num_errors++;
                    }
                }

                if (status == VX_SUCCESS)
                {
                    for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
                    {
                        if ((graph->nodes[n]->parameters[p] != NULL) &&
                            (graph->nodes[n]->kernel->signature.directions[p] == VX_OUTPUT))
                        {
                            if (postprocess_output(graph, n, p, vref[p], metas[p], &status, &num_errors) == vx_false_e)
                                break;
                        }
                    }
                }

                for (p = 0; p < dimof(metas); p++)
                {
                    if (metas[p])
                        ownReleaseMetaFormat(&metas[p]);
                }
            }
            else /* old style validators */
            {
                vx_meta_format meta = 0;

                /* first pass for inputs */
                for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
                {
                    if (((graph->nodes[n]->kernel->signature.directions[p] == VX_BIDIRECTIONAL) ||
                         (graph->nodes[n]->kernel->signature.directions[p] == VX_INPUT)) &&
                        (graph->nodes[n]->parameters[p] != NULL))
                    {
                        vx_status input_validation_status = graph->nodes[n]->kernel->validate_input((vx_node)graph->nodes[n], p);
                        if (input_validation_status != VX_SUCCESS)
                        {
                            status = input_validation_status;
                            vxAddLogEntry(&graph->base, status, "Node[%u] %s: parameter[%u] failed input/bi validation!\n",
                                          n, graph->nodes[n]->kernel->name,
                                          p);
                            VX_PRINT(VX_ZONE_GRAPH,"Failed on validation of parameter %u of kernel %s in node #%d (status=%d)\n",
                                     p, graph->nodes[n]->kernel->name, n, status);
                            num_errors++;
                        }
                    }
                }
                /* second pass for bi/output (we may encounter "virtual" objects here,
                 * then we must reparse graph to replace with new objects)
                 */
                /*! \bug Bidirectional parameters currently break parsing. */
                for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
                {
                    vx_reference_t *vref = NULL;
                    if (graph->nodes[n]->parameters[p] == NULL)
                        continue;

                    VX_PRINT(VX_ZONE_GRAPH,"Checking Node[%u].Parameter[%u]\n", n, p);
                    if (graph->nodes[n]->kernel->signature.directions[p] == VX_OUTPUT)
                    {
                        vx_status output_validation_status = VX_SUCCESS;
                        if (setup_output(graph, n, p, &vref, &meta, &status, &num_errors) == vx_false_e)
                            break;
                        output_validation_status = graph->nodes[n]->kernel->validate_output((vx_node)graph->nodes[n], p, meta);
                        if (output_validation_status == VX_SUCCESS)
                        {
                            if (postprocess_output(graph, n, p, vref, meta, &status, &num_errors) == vx_false_e)
                                break;
                        }
                        else
                        {
                            status = output_validation_status;
                            vxAddLogEntry(&graph->base, status, "Node %s: parameter[%u] failed output validation! (status = %d)\n",
                                          graph->nodes[n]->kernel->name, p, status);
                            VX_PRINT(VX_ZONE_ERROR,"Failed on validation of output parameter[%u] on kernel %s, status=%d\n",
                                     p,
                                     graph->nodes[n]->kernel->name,
                                     status);
                        }
                    }
                    if (meta)
                       ownReleaseMetaFormat(&meta);
                }
                if (meta)
                    ownReleaseMetaFormat(&meta);
            }
        }

        VX_PRINT(VX_ZONE_GRAPH,"####################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Single Writer Phase! (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"####################\n");

        for (n = 0; (n < graph->numNodes) && (status == VX_SUCCESS); n++)
        {
            for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
            {
                if (graph->nodes[n]->parameters[p] &&
                    ((graph->nodes[n]->kernel->signature.directions[p] == VX_OUTPUT) ||
                     (graph->nodes[n]->kernel->signature.directions[p] == VX_BIDIRECTIONAL)))
                {
                    vx_uint32 n1, p1;
                    /* check for other output references to this parameter in the graph. */
                    for (n1 = vxNextNode(graph, n); n1 != n; n1=vxNextNode(graph, n1))
                    {
                        for (p1 = 0; p1 < graph->nodes[n]->kernel->signature.num_parameters; p1++)
                        {
                            if ((graph->nodes[n1]->kernel->signature.directions[p1] == VX_OUTPUT) ||
                                 (graph->nodes[n1]->kernel->signature.directions[p1] == VX_BIDIRECTIONAL))
                            {
                                if (vxCheckWriteDependency(graph->nodes[n]->parameters[p], graph->nodes[n1]->parameters[p1]))
                                {
                                    status = VX_ERROR_MULTIPLE_WRITERS;
                                    VX_PRINT(VX_ZONE_GRAPH, "Multiple Writer to a reference found, check log!\n");
                                    vxAddLogEntry(&graph->base, status, "Node %u and Node %u are trying to output to the same reference "VX_FMT_REF"\n", n, n1, graph->nodes[n]->parameters[p]);
                                }
                            }
                        }
                    }
                }
            }
        }

        VX_PRINT(VX_ZONE_GRAPH,"########################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Memory Allocation Phase! (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"########################\n");

        /* now make sure each parameter is backed by memory. */
        for (n = 0; (n < graph->numNodes) && (status == VX_SUCCESS); n++)
        {
            VX_PRINT(VX_ZONE_GRAPH,"Checking node %u\n",n);

            for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++)
            {
                if (graph->nodes[n]->parameters[p])
                {
                    VX_PRINT(VX_ZONE_GRAPH,"\tparameter[%u]=%p type %d sig type %d\n", p,
                                 graph->nodes[n]->parameters[p],
                                 graph->nodes[n]->parameters[p]->type,
                                 graph->nodes[n]->kernel->signature.types[p]);

                    if (graph->nodes[n]->parameters[p]->type == VX_TYPE_IMAGE)
                    {
                        if (ownAllocateImage((vx_image_t *)graph->nodes[n]->parameters[p]) == vx_false_e)
                        {
                            vxAddLogEntry(&graph->base, VX_ERROR_NO_MEMORY, "Failed to allocate image at node[%u] %s parameter[%u]\n",
                                n, graph->nodes[n]->kernel->name, p);
                            VX_PRINT(VX_ZONE_ERROR, "See log\n");
                        }
                    }
                    else if ((VX_TYPE_IS_SCALAR(graph->nodes[n]->parameters[p]->type)) ||
                             (graph->nodes[n]->parameters[p]->type == VX_TYPE_RECTANGLE) ||
                             (graph->nodes[n]->parameters[p]->type == VX_TYPE_THRESHOLD))
                    {
                        /* these objects don't need to be allocated */
                    }
                    else if (graph->nodes[n]->parameters[p]->type == VX_TYPE_LUT)
                    {
                        vx_lut_t *lut = (vx_lut_t *)graph->nodes[n]->parameters[p];
                        if (ownAllocateMemory(graph->base.context, &lut->memory) == vx_false_e)
                        {
                            vxAddLogEntry(&graph->base, VX_ERROR_NO_MEMORY, "Failed to allocate lut at node[%u] %s parameter[%u]\n",
                                n, graph->nodes[n]->kernel->name, p);
                            VX_PRINT(VX_ZONE_ERROR, "See log\n");
                        }
                    }
                    else if (graph->nodes[n]->parameters[p]->type == VX_TYPE_DISTRIBUTION)
                    {
                        vx_distribution_t *dist = (vx_distribution_t *)graph->nodes[n]->parameters[p];
                        if (ownAllocateMemory(graph->base.context, &dist->memory) == vx_false_e)
                        {
                            vxAddLogEntry(&graph->base, VX_ERROR_NO_MEMORY, "Failed to allocate distribution at node[%u] %s parameter[%u]\n",
                                n, graph->nodes[n]->kernel->name, p);
                            VX_PRINT(VX_ZONE_ERROR, "See log\n");
                        }
                    }
                    else if (graph->nodes[n]->parameters[p]->type == VX_TYPE_PYRAMID)
                    {
                        vx_pyramid_t *pyr = (vx_pyramid_t *)graph->nodes[n]->parameters[p];
                        vx_uint32 i = 0;
                        for (i = 0; i < pyr->numLevels; i++)
                        {
                            if (ownAllocateImage((vx_image_t *)pyr->levels[i]) == vx_false_e)
                            {
                                vxAddLogEntry(&graph->base, VX_ERROR_NO_MEMORY, "Failed to allocate pyramid image at node[%u] %s parameter[%u]\n",
                                    n, graph->nodes[n]->kernel->name, p);
                                VX_PRINT(VX_ZONE_ERROR, "See log\n");
                            }
                        }
                    }
                    else if ((graph->nodes[n]->parameters[p]->type == VX_TYPE_MATRIX) ||
                              (graph->nodes[n]->parameters[p]->type == VX_TYPE_CONVOLUTION))
                    {
                        vx_matrix_t *mat = (vx_matrix_t *)graph->nodes[n]->parameters[p];
                        if (ownAllocateMemory(graph->base.context, &mat->memory) == vx_false_e)
                        {
                            vxAddLogEntry(&graph->base, VX_ERROR_NO_MEMORY, "Failed to allocate matrix (or subtype) at node[%u] %s parameter[%u]\n",
                                n, graph->nodes[n]->kernel->name, p);
                            VX_PRINT(VX_ZONE_ERROR, "See log\n");
                        }
                    }
                    else if (graph->nodes[n]->kernel->signature.types[p] == VX_TYPE_ARRAY)
                    {
                        if (ownAllocateArray((vx_array_t *)graph->nodes[n]->parameters[p]) == vx_false_e)
                        {
                            vxAddLogEntry(&graph->base, VX_ERROR_NO_MEMORY, "Failed to allocate array at node[%u] %s parameter[%u]\n",
                                n, graph->nodes[n]->kernel->name, p);
                            VX_PRINT(VX_ZONE_ERROR, "See log\n");
                        }
                    }
                    /*! \todo add other memory objects to graph auto-allocator as needed! */
                }
            }
        }

        VX_PRINT(VX_ZONE_GRAPH,"###############################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Head Nodes Determination Phase! (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"###############################\n");

        memset(graph->heads, 0, sizeof(graph->heads));
        graph->numHeads = 0;

        /* now traverse the graph and put nodes with no predecessor in the head list */
        for (n = 0; (n < graph->numNodes) && (status == VX_SUCCESS); n++)
        {
            uint32_t n1,p1;
            vx_bool isAHead = vx_true_e; /* assume every node is a head until proven otherwise */

            for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters && isAHead == vx_true_e; p++)
            {
                if ((graph->nodes[n]->kernel->signature.directions[p] == VX_INPUT) &&
                    (graph->nodes[n]->parameters[p] != NULL))
                {
                    /* ring loop over the node array, checking every node but this nth node. */
                    for (n1 = vxNextNode(graph, n);
                         (n1 != n) && (isAHead == vx_true_e);
                         n1 = vxNextNode(graph, n1))
                    {
                        for (p1 = 0; p1 < graph->nodes[n1]->kernel->signature.num_parameters && isAHead == vx_true_e; p1++)
                        {
                            if (graph->nodes[n1]->kernel->signature.directions[p1] != VX_INPUT)
                            {
                                VX_PRINT(VX_ZONE_GRAPH,"Checking input nodes[%u].parameter[%u] to nodes[%u].parameters[%u]\n", n, p, n1, p1);
                                /* if the parameter is referenced elsewhere */
                                if (vxCheckWriteDependency(graph->nodes[n]->parameters[p], graph->nodes[n1]->parameters[p1]))
                                {
                                    VX_PRINT(VX_ZONE_GRAPH,"\tnodes[%u].parameter[%u] referenced in nodes[%u].parameter[%u]\n", n,p,n1,p1);
                                    isAHead = vx_false_e; /* this will cause all the loops to break too. */
                                }
                            }
                        }
                    }
                }
            }

            if (isAHead == vx_true_e)
            {
                VX_PRINT(VX_ZONE_GRAPH,"Found a head in node[%u] => %s\n", n, graph->nodes[n]->kernel->name);
                graph->heads[graph->numHeads++] = n;
            }
        }

        /* graph has a cycle as there are no starting points! */
        if ((graph->numHeads == 0) && (status == VX_SUCCESS))
        {
            status = VX_ERROR_INVALID_GRAPH;
            VX_PRINT(VX_ZONE_ERROR,"Graph has no heads!\n");
            vxAddLogEntry(&graph->base, status, "Cycle: Graph has no head nodes!\n");
        }

        VX_PRINT(VX_ZONE_GRAPH,"##############\n");
        VX_PRINT(VX_ZONE_GRAPH,"Cycle Checking (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"##############\n");

        ownClearVisitation(graph);

        /* cycle checking by traversal of the graph from heads to tails */
        for (h = 0; h < graph->numHeads; h++)
        {
            vx_status cycle_status = ownTraverseGraph(graph, VX_INT_MAX_NODES, graph->heads[h]);
            if (cycle_status != VX_SUCCESS)
            {
                status = cycle_status;
                VX_PRINT(VX_ZONE_ERROR,"Cycle found in graph!");
                vxAddLogEntry(&graph->base, status, "Cycle: Graph has a cycle!\n");
                goto exit;
            }
        }

        VX_PRINT(VX_ZONE_GRAPH,"############################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Checking for Unvisited Nodes (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"############################\n");

        for (n = 0; (n < graph->numNodes) && (status == VX_SUCCESS); n++)
        {
            if (graph->nodes[n]->visited == vx_false_e)
            {
                VX_PRINT(VX_ZONE_ERROR, "UNVISITED: %s node[%u]\n", graph->nodes[n]->kernel->name, n);
                status = VX_ERROR_INVALID_GRAPH;
                vxAddLogEntry(&graph->base, status, "Node %s: unvisited!\n", graph->nodes[n]->kernel->name);
            }
        }

        ownClearVisitation(graph);

        if (hasACycle == vx_true_e)
        {
            status = VX_ERROR_INVALID_GRAPH;
            vxAddLogEntry(&graph->base, status, "Cycle: Graph has a cycle!\n");
            goto exit;
        }

        VX_PRINT(VX_ZONE_GRAPH,"#########################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Target Verification Phase (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"#########################\n");

        for (n = 0; (n < graph->numNodes) && (status == VX_SUCCESS); n++)
        {
            vx_uint32 index = graph->nodes[n]->affinity;
            vx_target_t *target = &graph->base.context->targets[index];
            if (target)
            {
                vx_status target_verify_status = target->funcs.verify(target, graph->nodes[n]);
                if (target_verify_status != VX_SUCCESS)
                {
                    status = target_verify_status;
                    vxAddLogEntry(&graph->base, status, "Target: %s Failed to Verify Node %s\n", target->name, graph->nodes[n]->kernel->name);
                }
            }
        }

        VX_PRINT(VX_ZONE_GRAPH,"#######################\n");
        VX_PRINT(VX_ZONE_GRAPH,"Kernel Initialize Phase (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"#######################\n");

        for (n = 0; (n < graph->numNodes) && (status == VX_SUCCESS); n++)
        {
            vx_node_t *node = graph->nodes[n];
            if (node->kernel->initialize)
            {
                vx_status kernel_init_status = VX_FAILURE;

                /* call the kernel initialization routine */
                if ((node->kernel->user_kernel == vx_true_e) &&
                    (node->kernel->attributes.localDataSize == 0))
                    node->local_data_change_is_enabled = vx_true_e;

                kernel_init_status = node->kernel->initialize((vx_node)node,
                                                  (vx_reference *)node->parameters,
                                                  node->kernel->signature.num_parameters);
                node->local_data_change_is_enabled = vx_false_e;
                if (kernel_init_status != VX_SUCCESS)
                {
                    status = kernel_init_status;
                    vxAddLogEntry(&graph->base, status, "Kernel: %s failed to initialize!\n", node->kernel->name);
                }
            }

            /* once the kernel has been initialized, create any local data for it */
            if ((node->attributes.localDataSize > 0) &&
                (node->attributes.localDataPtr == NULL))
            {
                node->attributes.localDataPtr = calloc(1, node->attributes.localDataSize);
                if (node->kernel->user_kernel == vx_true_e)
                    node->local_data_set_by_implementation = vx_true_e;
                VX_PRINT(VX_ZONE_GRAPH, "Local Data Allocated "VX_FMT_SIZE" bytes for node into %p\n!",
                        node->attributes.localDataSize,
                        node->attributes.localDataPtr);
            }
#ifdef OPENVX_KHR_TILING
            /* if this is a tiling kernel, we can also have tile memory (the sample only makes 1 buffer) */
            if ((node->attributes.tileDataSize > 0) &&
                (node->attributes.tileDataPtr == NULL))
            {
                node->attributes.tileDataPtr = calloc(1, node->attributes.tileDataSize);
            }
#endif
        }

        VX_PRINT(VX_ZONE_GRAPH,"#######################\n");
        VX_PRINT(VX_ZONE_GRAPH,"COST CALCULATIONS (%d)\n", status);
        VX_PRINT(VX_ZONE_GRAPH,"#######################\n");
        for (n = 0; (n < graph->numNodes) && (status == VX_SUCCESS); n++) {
            graph->nodes[n]->costs.bandwidth = 0ul;
            for (p = 0; p < graph->nodes[n]->kernel->signature.num_parameters; p++) {
                vx_reference ref = graph->nodes[n]->parameters[p];
                if (ref) {
                    vx_uint32 i;
                    switch (ref->type) {
                        case VX_TYPE_IMAGE:
                        {
                            vx_image image = (vx_image)ref;
                            for (i = 0; i < image->memory.nptrs; i++)
                                graph->nodes[n]->costs.bandwidth += ownComputeMemorySize(&image->memory, i);
                            break;
                        }
                        case VX_TYPE_ARRAY:
                        {
                            vx_array array = (vx_array)ref;
                            graph->nodes[n]->costs.bandwidth += ownComputeMemorySize(&array->memory, 0);
                            break;
                        }
                        case VX_TYPE_PYRAMID:
                        {
                            vx_pyramid pyr = (vx_pyramid)ref;
                            vx_uint32 j;
                            for (j = 0; j < pyr->numLevels; j++) {
                                vx_image image = pyr->levels[j];
                                for (i = 0; i < image->memory.nptrs; i++)
                                    graph->nodes[n]->costs.bandwidth += ownComputeMemorySize(&image->memory, i);
                            }
                            break;
                        }
                        default:
                            //VX_PRINT(VX_ZONE_WARNING, "Node[%u].parameter[%u] Unknown bandwidth cost!\n", n, p);
                            break;
                    }
                }
            }
            VX_PRINT(VX_ZONE_GRAPH, "Node[%u] has bandwidth cost of "VX_FMT_SIZE" bytes\n", n, graph->nodes[n]->costs.bandwidth);
        }

exit:
        graph->reverify = vx_false_e;
        if (status == VX_SUCCESS)
        {
            graph->verified = vx_true_e;
            graph->state = VX_GRAPH_STATE_VERIFIED;
        }
        else
        {
            graph->verified = vx_false_e;
            graph->state = VX_GRAPH_STATE_UNVERIFIED;
        }

        /* unlock the graph */
        ownSemPost(&graph->base.lock);
    }
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    VX_PRINT(VX_ZONE_GRAPH,"Returning status %d\n", status);

    return status;
}

static vx_status vxExecuteGraph(vx_graph graph, vx_uint32 depth)
{
    vx_status status = VX_SUCCESS;
    vx_action action = VX_ACTION_CONTINUE;
    vx_uint32 n, p, numLast, numNext, numLeft = 0;
    vx_uint32 last_nodes[VX_INT_MAX_REF];
    vx_uint32 next_nodes[VX_INT_MAX_REF];
    vx_uint32 left_nodes[VX_INT_MAX_REF];
    vx_context context = vxGetContext((vx_reference)graph);
    (void)depth;

#if defined(OPENVX_USE_SMP)
    vx_value_set_t workitems[VX_INT_MAX_REF];
#endif
    if (ownIsValidReference(&graph->base) == vx_false_e)
    {
        return VX_ERROR_INVALID_REFERENCE;
    }
    if (graph->verified == vx_false_e)
    {
        status = vxVerifyGraph((vx_graph)graph);
        if (status != VX_SUCCESS)
        {
            return status;
        }
    }
    VX_PRINT(VX_ZONE_GRAPH,"************************\n");
    VX_PRINT(VX_ZONE_GRAPH,"*** PROCESSING GRAPH ***\n");
    VX_PRINT(VX_ZONE_GRAPH,"************************\n");

    graph->state = VX_GRAPH_STATE_RUNNING;
    ownClearVisitation(graph);
    ownClearExecution(graph);
    if (context->perf_enabled)
    {
        ownStartCapture(&graph->perf);
    }
    /* initialize the next_nodes as the graph heads */
    memcpy(next_nodes, graph->heads, graph->numHeads * sizeof(vx_uint32));
    numNext = graph->numHeads;

    do {
        for (n = 0; n < numNext; n++)
        {
            ownPrintNode(graph->nodes[next_nodes[n]]);
        }

        /* execute the next nodes */
        for (n = 0; n < numNext; n++)
        {
            if (graph->nodes[next_nodes[n]]->executed == vx_false_e)
            {
                vx_uint32 t = graph->nodes[next_nodes[n]]->affinity;
#if defined(OPENVX_USE_SMP)
                if (depth == 1 && graph->should_serialize == vx_false_e)
                {
                    vx_value_set_t *work = &workitems[n];
                    vx_target target = &graph->base.context->targets[t];
                    vx_node node = graph->nodes[next_nodes[n]];
                    work->v1 = (vx_value_t)target;
                    work->v2 = (vx_value_t)node;
                    work->v3 = (vx_value_t)VX_ACTION_CONTINUE;
                    VX_PRINT(VX_ZONE_GRAPH, "Scheduling work on %s for %s\n", target->name, node->kernel->name);
                }
                else
#endif
                {
                    vx_target_t *target = &graph->base.context->targets[t];
                    vx_node_t *node = graph->nodes[next_nodes[n]];

                    /* turn on access to virtual memory */
                    for (p = 0u; p < node->kernel->signature.num_parameters; p++) {
                        if (node->parameters[p] == NULL) continue;
                        if (node->parameters[p]->is_virtual == vx_true_e) {
                            node->parameters[p]->is_accessible = vx_true_e;
                        }
                    }

                    VX_PRINT(VX_ZONE_GRAPH, "Calling Node[%u] %s:%s\n",
                             next_nodes[n],
                             target->name, node->kernel->name);

                    action = target->funcs.process(target, &node, 0, 1);

                    VX_PRINT(VX_ZONE_GRAPH, "Returned Node[%u] %s:%s Action %d\n",
                             next_nodes[n],
                             target->name, node->kernel->name,
                             action);

                    /* turn off access to virtual memory */
                    for (p = 0u; p < node->kernel->signature.num_parameters; p++) {
                        if (node->parameters[p] == NULL) continue;
                        if (node->parameters[p]->is_virtual == vx_true_e) {
                            node->parameters[p]->is_accessible = vx_false_e;
                        }
                    }

                    if (action == VX_ACTION_ABANDON)
                    {
                        break;
                    }
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Multiple executions attempted!\n");
                break;
            }
        }

#if defined(OPENVX_USE_SMP)
        if (depth == 1 && graph->should_serialize == vx_false_e)
        {
            if (ownIssueThreadpool(graph->base.context->workers, workitems, numNext) == vx_true_e)
            {
                /* do a blocking complete */
                VX_PRINT(VX_ZONE_GRAPH, "Issued %u work items!\n", numNext);
                if (ownCompleteThreadpool(graph->base.context->workers, vx_true_e) == vx_true_e)
                {
                    VX_PRINT(VX_ZONE_GRAPH, "Processed %u items in threadpool!\n", numNext);
                }
                action = VX_ACTION_CONTINUE;
                for (n = 0; n < numNext; n++)
                {
                    vx_action a = workitems[n].v3;
                    if (a != VX_ACTION_CONTINUE)
                    {
                        action = a;
                        VX_PRINT(VX_ZONE_WARNING, "Workitem[%u] returned action code %d\n", n, a);
                        break;
                    }
                }
            }
        }
#endif

        if (action == VX_ACTION_ABANDON)
        {
            break;
        }

        /* copy next_nodes to last_nodes */
        memcpy(last_nodes, next_nodes, numNext * sizeof(vx_uint32));
        numLast = numNext;

        /* determine the next nodes */
        ownFindNextNodes(graph, last_nodes, numLast, next_nodes, &numNext, left_nodes, &numLeft);

    } while (numNext > 0);

    if (action == VX_ACTION_ABANDON)
    {
        status = VX_ERROR_GRAPH_ABANDONED;
    }
    if (context->perf_enabled)
    {
        ownStopCapture(&graph->perf);
    }
    ownClearVisitation(graph);

    for (n = 0; n < VX_INT_MAX_REF; n++)
    {
        if (graph->delays[n] && ownIsValidSpecificReference(&graph->delays[n]->base, VX_TYPE_DELAY) == vx_true_e)
            vxAgeDelay(graph->delays[n]);
    }

    VX_PRINT(VX_ZONE_GRAPH,"Process returned status %d\n", status);
    if (context->perf_enabled)
    {
        for (n = 0; n < graph->numNodes; n++)
        {
            VX_PRINT(VX_ZONE_PERF,"nodes[%u] %s[%d] last:"VX_FMT_TIME"ms avg:"VX_FMT_TIME"ms min:"VX_FMT_TIME "ms max:"VX_FMT_TIME"\n",
                     n,
                     graph->nodes[n]->kernel->name,
                     graph->nodes[n]->kernel->enumeration,
                     ownTimeToMS(graph->nodes[n]->perf.tmp),
                     ownTimeToMS(graph->nodes[n]->perf.avg),
                     ownTimeToMS(graph->nodes[n]->perf.min),
                     ownTimeToMS(graph->nodes[n]->perf.max)
                     );
        }
    }

    if (status == VX_SUCCESS)
        graph->state = VX_GRAPH_STATE_COMPLETED;
    else
        graph->state = VX_GRAPH_STATE_ABANDONED;

    return status;
}

static vx_value_set_t graph_queue[10];
static vx_size numGraphsQueued = 0ul;

VX_API_ENTRY vx_status VX_API_CALL vxScheduleGraph(vx_graph graph)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidReference(&graph->base) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (ownSemTryWait(&graph->lock) == vx_true_e)
    {
        vx_sem_t* p_graph_queue_lock = graph->base.context->p_global_lock;
        vx_uint32 q = 0u;
        vx_value_set_t *pq = NULL;

        ownSemWait(p_graph_queue_lock);
        /* acquire a position in the graph queue */
        for (q = 0; q < dimof(graph_queue); q++) {
            if (graph_queue[q].v1 == 0) {
                pq = &graph_queue[q];
                numGraphsQueued++;
                break;
            }
        }
        ownSemPost(p_graph_queue_lock);
        if (pq)
        {
            memset(pq, 0, sizeof(vx_value_set_t));
            pq->v1 = (vx_value_t)graph;
            /* now add the graph to the queue */
            VX_PRINT(VX_ZONE_GRAPH,"Writing graph=" VX_FMT_REF ", status=%d\n",graph, status);
            if (ownWriteQueue(&graph->base.context->proc.input, pq) == vx_true_e)
            {
                status = VX_SUCCESS;
            }
            else
            {
                ownSemPost(&graph->lock);
                status = VX_ERROR_NO_RESOURCES;
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Graph queue is full\n");
            status = VX_ERROR_NO_RESOURCES;
        }
    }
    else
    {
        /* graph is already scheduled */
        status = VX_ERROR_GRAPH_SCHEDULED;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxWaitGraph(vx_graph graph)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidReference(&graph->base) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (ownSemTryWait(&graph->lock) == vx_false_e) // locked
    {
        vx_sem_t* p_graph_queue_lock = graph->base.context->p_global_lock;
        vx_graph g2;
        vx_bool ret = vx_true_e;
        vx_value_set_t *data = NULL;
        do
        {
            ret = ownReadQueue(&graph->base.context->proc.output, &data);
            if (ret == vx_false_e)
            {
                /* graph was locked but the queue was empty... */
                VX_PRINT(VX_ZONE_ERROR, "Queue was empty but graph was locked.\n");
                status = VX_FAILURE;
            }
            else
            {
                g2 = (vx_graph)data->v1;
                status = (vx_status)data->v2;
                if (g2 == graph) /* great, it's the graph we want. */
                {
                    vx_uint32 q = 0u;
                    ownSemWait(p_graph_queue_lock);
                    /* find graph in the graph queue */
                    for (q = 0; q < dimof(graph_queue); q++)
                    {
                        if (graph_queue[q].v1 == (vx_value_t)graph)
                        {
                            graph_queue[q].v1 = 0;
                            numGraphsQueued--;
                            break;
                        }
                    }
                    ownSemPost(p_graph_queue_lock);
                    break;
                }
                else
                {
                    /* not the right graph, put it back. */
                    ownWriteQueue(&graph->base.context->proc.output, data);
                }
            }
        } while (ret == vx_true_e);
        ownSemPost(&graph->lock); /* unlock the graph. */
    }
    else
    {
        status = VX_FAILURE;
        ownSemPost(&graph->lock); /* was free, release */
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxProcessGraph(vx_graph graph)
{
    if (ownIsValidReference(&graph->base) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    {
        /* create a counter for re-entrancy checking */
        static vx_uint32 count = 0;
        vx_sem_t* p_sem = graph->base.context->p_global_lock;
        vx_status status = VX_SUCCESS;

        ownSemWait(p_sem);
        count++;
        ownSemPost(p_sem);
        status = vxExecuteGraph(graph, count);
        ownSemWait(p_sem);
        count--;
        ownSemPost(p_sem);

        return status;
    }
}

VX_API_ENTRY vx_status VX_API_CALL vxAddParameterToGraph(vx_graph graph, vx_parameter param)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if ((ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e) &&
        (ownIsValidSpecificReference(&param->base, VX_TYPE_PARAMETER) == vx_true_e))
    {
        graph->parameters[graph->numParams].node = param->node;
        graph->parameters[graph->numParams].index = param->index;
        graph->numParams++;
        status = VX_SUCCESS;
    }
    else if ((ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e) &&
              (ownIsValidSpecificReference(&param->base, VX_TYPE_PARAMETER) == vx_false_e))
    {
        /* insert an empty parameter */
        graph->parameters[graph->numParams].node = NULL;
        graph->parameters[graph->numParams].index = 0;
        graph->numParams++;
        status = VX_SUCCESS;
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Graph "VX_FMT_REF" was invalid!\n", graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetGraphParameterByIndex(vx_graph graph, vx_uint32 index, vx_reference value)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e)
    {
        if (index < VX_INT_MAX_PARAMS)
        {
            status = vxSetParameterByIndex((vx_node)graph->parameters[index].node,
                                           graph->parameters[index].index,
                                           value);
        }
        else
        {
            status = VX_ERROR_INVALID_VALUE;
        }
    }
    return status;
}

VX_API_ENTRY vx_parameter VX_API_CALL vxGetGraphParameterByIndex(vx_graph graph, vx_uint32 index)
{
    vx_parameter parameter = 0;
    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e)
    {
        if ((index < VX_INT_MAX_PARAMS) && (index < graph->numParams))
        {
            vx_uint32 node_index = graph->parameters[index].index;
            parameter = vxGetParameterByIndex((vx_node)graph->parameters[index].node, node_index);
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid Graph!\n");
        vxAddLogEntry(&graph->base, VX_ERROR_INVALID_REFERENCE, "Invalid Graph given to %s\n", __FUNCTION__);
    }
    return parameter;
}

VX_API_ENTRY vx_bool VX_API_CALL vxIsGraphVerified(vx_graph graph)
{
    vx_bool verified = vx_false_e;
    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e)
    {
        VX_PRINT(VX_ZONE_GRAPH, "Graph is %sverified\n", (graph->verified == vx_true_e?"":"NOT "));
        verified = graph->verified;
    }
    return verified;
}
