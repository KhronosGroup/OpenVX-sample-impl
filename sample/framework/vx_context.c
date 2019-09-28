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

#include <ctype.h>
#include "vx_internal.h"
#include "vx_context.h"

const vx_char implementation[VX_MAX_IMPLEMENTATION_NAME] = "khronos.sample";

vx_char targetModules[][VX_MAX_TARGET_NAME] = {
    "openvx-c_model",
};

const vx_char extensions[] =
#if defined(OPENVX_USE_TILING)
    OPENVX_KHR_TILING" "
#endif
#if defined(OPENVX_USE_XML)
    OPENVX_KHR_XML" "
#endif
#if defined(OPENVX_USE_S16)
    "vx_khr_s16 "
#endif
#if defined(EXPERIMENTAL_USE_DOT)
    OPENVX_KHR_DOT" "
#endif
    " ";

static vx_context_t *single_context = NULL;
static vx_sem_t context_lock;
static vx_sem_t global_lock;

static vx_bool vxWorkerNode(vx_threadpool_worker_t *worker)
{
    vx_bool ret = vx_true_e;
    vx_target target = (vx_target)worker->data->v1;
    vx_node node = (vx_node)worker->data->v2;
    vx_action action = (vx_action)worker->data->v3;
    vx_uint32 p = 0;

    /* turn on access to virtual memory */
    for (p = 0u; p < node->kernel->signature.num_parameters; p++) {
        if (node->parameters[p] == NULL) continue;
        if (node->parameters[p]->is_virtual == vx_true_e) {
            node->parameters[p]->is_accessible = vx_true_e;
        }
    }

    VX_PRINT(VX_ZONE_GRAPH, "Executing %s on target %s\n", node->kernel->name, target->name);
    action = target->funcs.process(target, &node, 0, 1);
    VX_PRINT(VX_ZONE_GRAPH, "Executed %s on target %s with action %d returned\n", node->kernel->name, target->name, action);

    /* turn on access to virtual memory */
    for (p = 0u; p < node->kernel->signature.num_parameters; p++) {
        if (node->parameters[p] == NULL) continue;
        if (node->parameters[p]->is_virtual == vx_true_e) {
            // determine who is the last thread to release...
            // if this is an input, then there should be "zero" rectangles, which
            // should allow commits to work, even if the flag is lowered.
            // if this is an output, there should only be a single writer, so
            // no locks are needed. Bidirectional is not allowed to be virtual.
            node->parameters[p]->is_accessible = vx_true_e;
        }
    }

    if (action == VX_ACTION_ABANDON)
    {
        ret = vx_false_e;
    }
    // collect the specific results.
    worker->data->v3 = (vx_value_t)action;
    return ret;
}

static vx_value_t vxWorkerGraph(void *arg)
{
    vx_processor_t *proc = (vx_processor_t *)arg;
    VX_PRINT(VX_ZONE_CONTEXT, "Starting thread!\n");
    while (proc->running == vx_true_e)
    {
        vx_graph g = 0;
        vx_status s = VX_FAILURE;
        vx_value_set_t *data = NULL;
        if (ownReadQueue(&proc->input, &data) == vx_true_e)
        {
            g = (vx_graph)data->v1;
            // s = (vx_status)v2;
            VX_PRINT(VX_ZONE_CONTEXT, "Read graph=" VX_FMT_REF ", status=%d\n",g,s);
            s = vxProcessGraph(g);
            VX_PRINT(VX_ZONE_CONTEXT, "Writing graph=" VX_FMT_REF ", status=%d\n",g,s);
            data->v1 = (vx_value_t)g;
            data->v2 = (vx_status)s;
            if (ownWriteQueue(&proc->output, data) == vx_false_e)
                VX_PRINT(VX_ZONE_ERROR, "Failed to write graph=" VX_FMT_REF " status=%d\n", g, s);
        }
    }
    VX_PRINT(VX_ZONE_CONTEXT,"Stopping thread!\n");
    return 0;
}

VX_INT_API vx_bool ownIsValidType(vx_enum type)
{
    vx_bool ret = vx_false_e;
    if (type <= VX_TYPE_INVALID)
    {
        ret = vx_false_e;
    }
    else if (VX_TYPE_IS_SCALAR(type)) /* some scalar */
    {
        ret = vx_true_e;
    }
    else if (VX_TYPE_IS_STRUCT(type)) /* some struct */
    {
        ret = vx_true_e;
    }
    else if (VX_TYPE_IS_OBJECT(type)) /* some object */
    {
        ret = vx_true_e;
    }
#ifdef OPENVX_KHR_XML
    else if (type == VX_TYPE_IMPORT) /* import type extension */
    {
        ret = vx_true_e;
    }
#endif
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Type 0x%08x is invalid!\n");
    }
    return ret; /* otherwise, not a valid type */
}

VX_INT_API vx_bool ownIsValidImport(vx_enum type)
{
    vx_bool ret = vx_false_e;
    switch(type)
    {
        case VX_MEMORY_TYPE_HOST:
            ret = vx_true_e;
            break;
#ifdef OPENVX_USE_OPENCL_INTEROP
        case VX_MEMORY_TYPE_OPENCL_BUFFER:
            if (single_context && single_context->opencl_context) {
                ret = vx_true_e;
            }
            break;
#endif
        case VX_MEMORY_TYPE_NONE:
        default:
            ret = vx_false_e;
            break;
    }
    return ret;
}

VX_INT_API vx_bool ownIsValidContext(vx_context context)
{
    vx_bool ret = vx_false_e;
    if ((context != NULL) &&
        (context->base.magic == VX_MAGIC) &&
        (context->base.type == VX_TYPE_CONTEXT) &&
        (context->base.context == NULL))
    {
        ret = vx_true_e; /* this is the top level context */
    }
    if (ret == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "%p is not a valid context!\n", context);
    }
    return ret;
}

VX_INT_API vx_bool vxIsValidBorderMode(vx_enum mode)
{
    vx_bool ret = vx_true_e;
    switch (mode)
    {
        case VX_BORDER_UNDEFINED:
        case VX_BORDER_CONSTANT:
        case VX_BORDER_REPLICATE:
            break;
        default:
            ret = vx_false_e;
            break;
    }
    return ret;
}

VX_INT_API vx_bool ownAddAccessor(vx_context context,
                                 vx_size size,
                                 vx_enum usage,
                                 void *ptr,
                                 vx_reference ref,
                                 vx_uint32 *pIndex,
                                 void *extra_data)
{
    vx_uint32 a;
    vx_bool worked = vx_false_e;
    for (a = 0u; a < dimof(context->accessors); a++)
    {
        if (context->accessors[a].used == vx_false_e)
        {
            VX_PRINT(VX_ZONE_CONTEXT, "Found open accessors[%u]\n", a);
            /* Allocation requested */
            if (size > 0ul && ptr == NULL)
            {
                context->accessors[a].ptr = malloc(size);
                if (context->accessors[a].ptr == NULL)
                    return vx_false_e;
                context->accessors[a].allocated = vx_true_e;
            }
            /* Pointer provided by the caller */
            else
            {
                context->accessors[a].ptr = ptr;
                context->accessors[a].allocated = vx_false_e;
            }
            context->accessors[a].usage = usage;
            context->accessors[a].ref = ref;
            context->accessors[a].used = vx_true_e;
            context->accessors[a].extra_data = extra_data;
            if (pIndex) *pIndex = a;
            worked = vx_true_e;
            break;
        }
    }
    return worked;
}

VX_INT_API vx_bool ownFindAccessor(vx_context context, const void *ptr, vx_uint32 *pIndex)
{
    vx_uint32 a;
    vx_bool worked = vx_false_e;
    for (a = 0u; a < dimof(context->accessors); a++)
    {
        if (context->accessors[a].used == vx_true_e)
        {
            if (context->accessors[a].ptr == ptr)
            {
                VX_PRINT(VX_ZONE_CONTEXT, "Found accessors[%u] for %p\n", a, ptr);
                worked = vx_true_e;
                if (pIndex) *pIndex = a;
                break;
            }
        }
    }
    return worked;
}

VX_INT_API void ownRemoveAccessor(vx_context context, vx_uint32 index)
{
    if (index < dimof(context->accessors))
    {
        if (context->accessors[index].allocated == vx_true_e)
        {
            free(context->accessors[index].ptr);
        }
        if (context->accessors[index].extra_data != NULL)
        {
            free(context->accessors[index].extra_data);
        }
        memset(&context->accessors[index], 0, sizeof(vx_external_t));
        VX_PRINT(VX_ZONE_CONTEXT, "Removed accessors[%u]\n", index);
    }
}

VX_INT_API vx_bool ownMemoryMap(
    vx_context   context,
    vx_reference ref,
    vx_size      size,
    vx_enum      usage,
    vx_enum      mem_type,
    vx_uint32    flags,
    void*        extra_data,
    void**       ptr,
    vx_map_id*   map_id)
{
    vx_uint32 id;
    vx_uint8* buf    = 0;
    vx_bool   worked = vx_false_e;

    /* lock the table for modification */
    if (vx_true_e == ownSemWait(&context->memory_maps_lock))
    {
        for (id = 0u; id < dimof(context->memory_maps); id++)
        {
            if (context->memory_maps[id].used == vx_false_e)
            {
                VX_PRINT(VX_ZONE_CONTEXT, "Found free memory map slot[%u]\n", id);

                /* allocate mapped buffer if requested (by providing size != 0) */
                if (size != 0)
                {
                    buf = malloc(size);
                    if (buf == NULL)
                    {
                        ownSemPost(&context->memory_maps_lock);
                        return vx_false_e;
                    }
                }

                context->memory_maps[id].used       = vx_true_e;
                context->memory_maps[id].ref        = ref;
                context->memory_maps[id].ptr        = buf;
                context->memory_maps[id].usage      = usage;
                context->memory_maps[id].mem_type   = mem_type;
                context->memory_maps[id].flags      = flags;

                vx_memory_map_extra* extra = (vx_memory_map_extra*)extra_data;
                if (VX_TYPE_IMAGE == ref->type)
                {
                    context->memory_maps[id].extra.image_data.plane_index = extra->image_data.plane_index;
                    context->memory_maps[id].extra.image_data.rect        = extra->image_data.rect;
                }
                else if (VX_TYPE_ARRAY == ref->type || VX_TYPE_LUT == ref->type)
                {
                    vx_memory_map_extra* extra = (vx_memory_map_extra*)extra_data;
                    context->memory_maps[id].extra.array_data.start = extra->array_data.start;
                    context->memory_maps[id].extra.array_data.end   = extra->array_data.end;
                }

                *ptr = buf;
                *map_id = (vx_map_id)id;

                worked = vx_true_e;

                break;
            }
        }

        /* we're done, unlock the table */
        worked = ownSemPost(&context->memory_maps_lock);
    }
    else
        worked = vx_false_e;

    return worked;
} /* ownMemoryMap() */

VX_INT_API vx_bool ownFindMemoryMap(
    vx_context   context,
    vx_reference ref,
    vx_map_id    map_id)
{
    vx_bool worked = vx_false_e;
    vx_uint32 id = (vx_uint32)map_id;

    /* check index range */
    if (id < dimof(context->memory_maps))
    {
        /* lock the table for exclusive access */
        if (vx_true_e == ownSemWait(&context->memory_maps_lock))
        {
            if ((context->memory_maps[id].used == vx_true_e) && (context->memory_maps[id].ref == ref))
            {
                worked = vx_true_e;
            }

            /* unlock the table */
            worked &= ownSemPost(&context->memory_maps_lock);
        }
    }

    return worked;
} /* ownFindMemoryMap() */

VX_INT_API void ownMemoryUnmap(vx_context context, vx_uint32 map_id)
{
    /* lock the table for modification */
    if (vx_true_e == ownSemWait(&context->memory_maps_lock))
    {
        if (context->memory_maps[map_id].used == vx_true_e)
        {
            if (context->memory_maps[map_id].ptr != NULL)
            {
                /* freeing mapped buffer */
                free(context->memory_maps[map_id].ptr);

                memset(&context->memory_maps[map_id], 0, sizeof(vx_memory_map_t));
            }
            VX_PRINT(VX_ZONE_CONTEXT, "Removed memory mapping[%u]\n", map_id);
        }

        context->memory_maps[map_id].used = vx_false_e;

        /* we're done, unlock the table */
        ownSemPost(&context->memory_maps_lock);
    }
    else
        VX_PRINT(VX_ZONE_ERROR, "ownSemWait() failed!\n");

    return;
} /* ownMemoryUnmap() */


/******************************************************************************/
/* PUBLIC API */
/******************************************************************************/

/* Note: context is an exception in term of error management since it
   returns 0 in case of error. This is due to the fact that error
   objects belong to the context in this implementation. But since
   vxGetStatus supports 0 in this implementation, this is consistent.
*/
#if !DISABLE_ICD_COMPATIBILITY
VX_API_ENTRY vx_context VX_API_CALL vxCreateContext(void)
{
    return vxCreateContextFromPlatform(NULL);
}

VX_API_ENTRY vx_context VX_API_CALL vxCreateContextFromPlatform(struct _vx_platform * platform)
#else
VX_API_ENTRY vx_context VX_API_CALL vxCreateContext(void)
#endif
{
    vx_context context = NULL;

    if (single_context == NULL)
    {
        ownCreateSem(&context_lock, 1);
        ownCreateSem(&global_lock, 1);
    }

    ownSemWait(&context_lock);
    if (single_context == NULL)
    {
        /* read the variables for debugging flags */
        vx_set_debug_zone_from_env();

        context = VX_CALLOC(vx_context_t); /* \todo get from allocator? */
        if (context)
        {
            vx_uint32 p = 0u, p2 = 0u, t = 0u, m = 0u;
#ifdef OPENVX_USE_OPENCL_INTEROP
            context->opencl_context = NULL;
            context->opencl_command_queue = NULL;
#endif
            context->p_global_lock = &global_lock;
            context->imm_border.mode = VX_BORDER_UNDEFINED;
            context->imm_border_policy = VX_BORDER_POLICY_DEFAULT_TO_UNDEFINED;
            context->next_dynamic_user_kernel_id = 0;
            context->next_dynamic_user_library_id = 1;
            context->perf_enabled = vx_false_e;
            ownInitReference(&context->base, NULL, VX_TYPE_CONTEXT, NULL);
#if !DISABLE_ICD_COMPATIBILITY
            context->base.platform = platform;
#endif
            ownIncrementReference(&context->base, VX_EXTERNAL);
            context->workers = ownCreateThreadpool(VX_INT_HOST_CORES,
                                                  VX_INT_MAX_REF, /* very deep queues! */
                                                  sizeof(vx_work_t),
                                                  vxWorkerNode,
                                                  context);
            ownCreateConstErrors(context);

            /* initialize modules */
            for (m = 0u; m < VX_INT_MAX_MODULES; m++)
            {
                ownCreateSem(&context->modules[m].lock, 1);
            }

            /* load all targets */
            for (t = 0u; t < dimof(targetModules); t++)
            {
                if (ownLoadTarget(context, targetModules[t]) == VX_SUCCESS)
                {
                    context->num_targets++;
                }
            }

            if (context->num_targets == 0)
            {
                VX_PRINT(VX_ZONE_ERROR, "No targets loaded!\n");
                free(context);
                ownSemPost(&context_lock);
                return 0;
            }

            /* initialize all targets */
            for (t = 0u; t < context->num_targets; t++)
            {
                if (context->targets[t].module.handle)
                {
                    /* call the init function */
                    if (context->targets[t].funcs.init(&context->targets[t]) != VX_SUCCESS)
                    {
                        VX_PRINT(VX_ZONE_WARNING, "Target %s failed to initialize!\n", context->targets[t].name);
                        /* unload this module */
                        ownUnloadTarget(context, t, vx_true_e);
                        break;
                    }
                    else
                    {
                        context->targets[t].enabled = vx_true_e;
                    }
                }
            }

            /* assign the targets by priority into the list */
            p2 = 0u;
            for (p = 0u; p < VX_TARGET_PRIORITY_MAX; p++)
            {
                for (t = 0u; t < context->num_targets; t++)
                {
                    vx_target_t * target = &context->targets[t];
                    if (p == target->priority)
                    {
                        context->priority_targets[p2] = t;
                        p2++;
                    }
                }
            }
            /* print out the priority list */
            for (t = 0u; t < context->num_targets; t++)
            {
                vx_target_t *target = &context->targets[context->priority_targets[t]];
                if (target->enabled == vx_true_e)
                {
                    VX_PRINT(VX_ZONE_TARGET, "target[%u]: %s\n",
                                target->priority,
                                target->name);
                }
            }

            // create the internal thread which processes graphs for asynchronous mode.
            ownInitQueue(&context->proc.input);
            ownInitQueue(&context->proc.output);
            context->proc.running = vx_true_e;
            context->proc.thread = ownCreateThread(vxWorkerGraph, &context->proc);
            single_context = context;
            context->imm_target_enum = VX_TARGET_ANY;
            memset(context->imm_target_string, 0, sizeof(context->imm_target_string));

            /* memory maps table lock */
            ownCreateSem(&context->memory_maps_lock, 1);
        }
    }
    else
    {
        context = single_context;
        ownIncrementReference(&context->base, VX_EXTERNAL);
    }
    ownSemPost(&context_lock);
    return (vx_context)context;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseContext(vx_context *c)
{
    vx_status status = VX_SUCCESS;
    vx_context context = (c?*c:0);
    vx_uint32 r,m,a;
    vx_uint32 t;

    if (c) *c = 0;
    ownSemWait(&context_lock);
    if (ownIsValidContext(context) == vx_true_e)
    {
        if (ownDecrementReference(&context->base, VX_EXTERNAL) == 0)
        {
            ownDestroyThreadpool(&context->workers);
            context->proc.running = vx_false_e;
            ownPopQueue(&context->proc.input);
            ownJoinThread(context->proc.thread, NULL);
            ownDeinitQueue(&context->proc.output);
            ownDeinitQueue(&context->proc.input);

            /* Deregister any log callbacks if there is any registered */
            vxRegisterLogCallback(context, NULL, vx_false_e);

            /*! \internal Garbage Collect All References */
            /* Details:
             *   1. This loop will warn of references which have not been released by the user.
             *   2. It will close all internally opened error references.
             *   3. It will close the external references, which in turn will internally
             *      close any internally dependent references that they reference, assuming the
             *      reference counting has been done properly in the framework.
             *   4. This garbage collection must be done before the targets are released since some of
             *      these external references may have internal references to target kernels.
             */
            for (r = 0; r < VX_INT_MAX_REF; r++)
            {
                vx_reference_t *ref = context->reftable[r];

                /* Warnings should only come when users have not released all external references */
                if (ref && ref->external_count > 0) {
                    VX_PRINT(VX_ZONE_WARNING,"Stale reference "VX_FMT_REF" of type %s at external count %u, internal count %u\n",
                             ref, ownGetObjectTypeName(ref->type), ref->external_count, ref->internal_count);
                }

                /* These were internally opened during creation, so should internally close ERRORs */
                if(ref && ref->type == VX_TYPE_ERROR) {
                    ownReleaseReferenceInt(&ref, ref->type, VX_INTERNAL, NULL);
                }

                /* Warning above so user can fix release external objects, but close here anyway */
                while (ref && ref->external_count > 1) {
                    ownDecrementReference(ref, VX_EXTERNAL);
                }
                if (ref && ref->external_count > 0) {
                    ownReleaseReferenceInt(&ref, ref->type, VX_EXTERNAL, NULL);
                }

            }

            for (m = 0; m < VX_INT_MAX_MODULES; m++)
            {
                ownSemWait(&context->modules[m].lock);
                if (context->modules[m].handle)
                {
                    ownUnloadModule(context->modules[m].handle);
                    memset(context->modules[m].name, 0, sizeof(context->modules[m].name));
                    context->modules[m].handle = VX_MODULE_INIT;
                }
                ownSemPost(&context->modules[m].lock);
                ownDestroySem(&context->modules[m].lock);
            }

            /* de-initialize and unload each target */
            for (t = 0u; t < context->num_targets; t++)
            {
                if (context->targets[t].enabled == vx_true_e)
                {
                    context->targets[t].funcs.deinit(&context->targets[t]);
                    ownUnloadTarget(context, t, vx_true_e);
                    context->targets[t].enabled = vx_false_e;
                }
            }

            /* Remove all outstanding accessors. */
            for (a = 0; a < dimof(context->accessors); ++a)
                if (context->accessors[a].used)
                    ownRemoveAccessor(context, a);

            /* Check for outstanding mappings */
            for (a = 0; a < dimof(context->memory_maps); ++a)
            {
                if (context->memory_maps[a].used)
                {
                    VX_PRINT(VX_ZONE_ERROR, "Memory map %d not unmapped\n", a);
                    ownMemoryUnmap(context, a);
                }
            }

            ownDestroySem(&context->memory_maps_lock);

            /* By now, all external and internal references should be removed */
            for (r = 0; r < VX_INT_MAX_REF; r++)
            {
                if(context->reftable[r])
                    VX_PRINT(VX_ZONE_ERROR,"Reference %d not removed\n", r);
            }

            /*! \internal wipe away the context memory first */
            /* Normally destroy sem is part of release reference, but can't for context */
            ownDestroySem(&((vx_reference )context)->lock);
            memset(context, 0, sizeof(vx_context_t));
            free((void *)context);
            ownDestroySem(&global_lock);
            ownSemPost(&context_lock);
            ownDestroySem(&context_lock);
            single_context = NULL;
            return status;
        }
        else
        {
            VX_PRINT(VX_ZONE_WARNING, "Context still has %u holders\n", ownTotalReferenceCount(&context->base));
        }
    } else {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    ownSemPost(&context_lock);
    return status;
}

VX_API_ENTRY vx_context VX_API_CALL vxGetContext(vx_reference reference)
{
    vx_context context = NULL;
    if (ownIsValidReference(reference) == vx_true_e)
    {
        context = reference->context;
    }
    else if (ownIsValidContext((vx_context)reference) == vx_true_e)
    {
        context = (vx_context)reference;
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "%p is not a valid reference\n", reference);
        VX_BACKTRACE(VX_ZONE_ERROR);
    }
    return context;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetContextAttribute(vx_context context, vx_enum attribute, const void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidContext(context) == vx_false_e)
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    else
    {
        switch (attribute) {
            case VX_CONTEXT_IMMEDIATE_BORDER:
                if (VX_CHECK_PARAM(ptr, size, vx_border_t, 0x3))
                {
                    vx_border_t *config = (vx_border_t *)ptr;
                    if (vxIsValidBorderMode(config->mode) == vx_false_e)
                        status = VX_ERROR_INVALID_VALUE;
                    else
                    {
                        context->imm_border = *config;
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
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryContext(vx_context context, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidContext(context) == vx_false_e)
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    else
    {
        switch (attribute)
        {
            case VX_CONTEXT_VENDOR_ID:
                if (VX_CHECK_PARAM(ptr, size, vx_uint16, 0x1))
                {
                    *(vx_uint16 *)ptr = VX_ID_KHRONOS;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_VERSION:
                if (VX_CHECK_PARAM(ptr, size, vx_uint16, 0x1))
                {
                    *(vx_uint16 *)ptr = (vx_uint16)VX_VERSION;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_MODULES:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = context->num_modules;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_REFERENCES:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = context->num_references;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_IMPLEMENTATION:
                if (size <= VX_MAX_IMPLEMENTATION_NAME && ptr)
                {
                    strncpy(ptr, implementation, VX_MAX_IMPLEMENTATION_NAME);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_EXTENSIONS_SIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = sizeof(extensions);
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_EXTENSIONS:
                if (size <= sizeof(extensions) && ptr)
                {
                    strncpy(ptr, extensions, sizeof(extensions));
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_CONVOLUTION_MAX_DIMENSION:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = VX_INT_MAX_CONVOLUTION_DIM;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_NONLINEAR_MAX_DIMENSION:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = VX_INT_MAX_NONLINEAR_DIM;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_OPTICAL_FLOW_MAX_WINDOW_DIMENSION:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = VX_OPTICALFLOWPYRLK_MAX_DIM;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_IMMEDIATE_BORDER:
                if (VX_CHECK_PARAM(ptr, size, vx_border_t, 0x3))
                {
                    *(vx_border_t *)ptr = context->imm_border;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_IMMEDIATE_BORDER_POLICY:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = context->imm_border_policy;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_UNIQUE_KERNELS:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = context->num_unique_kernels;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_MAX_TENSOR_DIMS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = VX_MAX_TENSOR_DIMENSIONS;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_UNIQUE_KERNEL_TABLE:
                if ((size == (context->num_unique_kernels * sizeof(vx_kernel_info_t))) &&
                    (ptr != NULL))
                {
                    vx_uint32 k = 0u;
                    vx_uint32 t = 0u;
                    vx_uint32 k2 = 0u;
                    vx_uint32 numk = 0u;
                    vx_kernel_info_t* table = (vx_kernel_info_t*)ptr;

                    for (t = 0; t < context->num_targets; t++)
                    {
                        for (k = 0u; k < VX_INT_MAX_KERNELS; k++)
                        {
                            vx_bool found = vx_false_e;
                            VX_PRINT(VX_ZONE_INFO, "Checking uniqueness of %s (%d)\n", context->targets[t].kernels[k].name, context->targets[t].kernels[k].enumeration);
                            for (k2 = 0u; k2 < numk; k2++)
                            {
                                if (VX_KERNEL_INVALID < context->targets[t].kernels[k].enumeration &&
                                    VX_KERNEL_MAX_1_0 > context->targets[t].kernels[k].enumeration &&
                                    table[k2].enumeration == context->targets[t].kernels[k].enumeration)
                                {
                                    found = vx_true_e;
                                    break;
                                }
                            }

                            if (VX_KERNEL_INVALID < context->targets[t].kernels[k].enumeration &&
                                VX_KERNEL_MAX_1_0 > context->targets[t].kernels[k].enumeration &&
                                found == vx_false_e)
                            {
                                VX_PRINT(VX_ZONE_INFO, "Kernel %s is unique\n", context->targets[t].kernels[k].name);
                                table[numk].enumeration = context->targets[t].kernels[k].enumeration;
                                strncpy(table[numk].name, context->targets[t].kernels[k].name, VX_MAX_KERNEL_NAME);
                                numk++;
                            }
                        }
                    }
                } else {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
#ifdef OPENVX_USE_OPENCL_INTEROP
            case VX_CONTEXT_CL_CONTEXT:
                if (VX_CHECK_PARAM(ptr, size, cl_context, 0x3))
                {
                    *(cl_context *)ptr = context->opencl_context;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_CONTEXT_CL_COMMAND_QUEUE:
                if (VX_CHECK_PARAM(ptr, size, cl_command_queue, 0x3))
                {
                    *(cl_command_queue  *)ptr = context->opencl_command_queue;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
#endif
            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxHint(vx_reference reference, vx_enum hint, const void* data, vx_size data_size)
{
    vx_status status = VX_SUCCESS;
    (void)data;
    (void)data_size;

    /* reference param should be a valid OpenVX reference*/
    if (ownIsValidContext((vx_context)reference) == vx_false_e && ownIsValidReference(reference) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    switch (hint)
    {
        /*! \todo add hints to the sample implementation */

        default:
            status = VX_ERROR_NOT_SUPPORTED;
            break;
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxDirective(vx_reference reference, vx_enum directive) {
    vx_status status = VX_SUCCESS;
    vx_context context;
    vx_enum ref_type;
    if (ownIsValidReference(reference) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;
    vxQueryReference(reference, VX_REFERENCE_TYPE, &ref_type, sizeof(ref_type));
    if (ref_type == VX_TYPE_CONTEXT)
    {
        context = (vx_context)reference;
    }
    else
    {
        context = reference->context;
    }
    if (ownIsValidContext(context) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;
    switch (directive)
    {
        case VX_DIRECTIVE_DISABLE_LOGGING:
            context->log_enabled = vx_false_e;
            break;
        case VX_DIRECTIVE_ENABLE_LOGGING:
            context->log_enabled = vx_true_e;
            break;
        case VX_DIRECTIVE_DISABLE_PERFORMANCE:
            if (ref_type == VX_TYPE_CONTEXT)
            {
                context->perf_enabled = vx_false_e;
            }
            else
            {
                status = VX_ERROR_NOT_SUPPORTED;
            }
            break;
        case VX_DIRECTIVE_ENABLE_PERFORMANCE:
            if (ref_type == VX_TYPE_CONTEXT)
            {
                context->perf_enabled = vx_true_e;
            }
            else
            {
                status = VX_ERROR_NOT_SUPPORTED;
            }
            break;
        default:
            status = VX_ERROR_NOT_SUPPORTED;
            break;
    }
    return status;
}

VX_API_ENTRY vx_enum VX_API_CALL vxRegisterUserStruct(vx_context context, vx_size size)
{
    vx_enum type = VX_TYPE_INVALID;
    vx_uint32 i = 0;

    if ((ownIsValidContext(context) == vx_true_e) &&
        (size != 0))
    {
        for (i = 0; i < VX_INT_MAX_USER_STRUCTS; ++i)
        {
            if (context->user_structs[i].type == VX_TYPE_INVALID)
            {
                context->user_structs[i].type = VX_TYPE_USER_STRUCT_START + i;
                context->user_structs[i].size = size;
                context->user_structs[i].name[0] = 0;
                type = context->user_structs[i].type;
                break;
            }
        }
    }
    return type;
}

VX_API_ENTRY vx_enum VX_API_CALL vxGetUserStructByName(vx_context context, const vx_char *name)
{
    vx_enum type = VX_TYPE_INVALID;
    vx_uint32 i = 0;
    vx_size len = strlen(name);
    if (VX_MAX_STRUCT_NAME < len)
        len = VX_MAX_STRUCT_NAME;
    if ((ownIsValidContext(context) == vx_true_e) &&
        (len != 0))
    {
        for (i = 0; i < VX_INT_MAX_USER_STRUCTS; ++i)
        {
            if ((VX_TYPE_INVALID != context->user_structs[i].type) &&
                (0 == strncmp(context->user_structs[i].name, name, len)))
            {
                type = context->user_structs[i].type;
                break;
            }
        }
    }
    return type;
}

VX_API_ENTRY vx_enum VX_API_CALL vxRegisterUserStructWithName(vx_context context, vx_size size, const vx_char *name)
{
    vx_enum type = VX_TYPE_INVALID;
    vx_uint32 i = 0;

    if ((ownIsValidContext(context) == vx_true_e) &&
        (size != 0) &&
        VX_TYPE_INVALID == vxGetUserStructByName(context, name))
    {
        for (i = 0; i < VX_INT_MAX_USER_STRUCTS; ++i)
        {
            if (context->user_structs[i].type == VX_TYPE_INVALID)
            {
                context->user_structs[i].type = VX_TYPE_USER_STRUCT_START + i;
                context->user_structs[i].size = size;
                strncpy(context->user_structs[i].name, name, VX_MAX_STRUCT_NAME - 1);
                type = context->user_structs[i].type;
                break;
            }
        }
    }
    return type;
}

VX_API_ENTRY vx_status VX_API_CALL vxAllocateUserKernelId(vx_context context, vx_enum * pKernelEnumId)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if ((ownIsValidContext(context) == vx_true_e) && pKernelEnumId)
    {
        status = VX_ERROR_NO_RESOURCES;
        if(context->next_dynamic_user_kernel_id <= VX_KERNEL_MASK)
        {
            *pKernelEnumId = VX_KERNEL_BASE(VX_ID_USER,0) + context->next_dynamic_user_kernel_id++;
            status = VX_SUCCESS;
        }
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxAllocateUserKernelLibraryId(vx_context context, vx_enum * pLibraryId)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if ((ownIsValidContext(context) == vx_true_e) && pLibraryId)
    {
        status = VX_ERROR_NO_RESOURCES;
        if(context->next_dynamic_user_library_id <= VX_LIBRARY(VX_LIBRARY_MASK))
        {
            *pLibraryId = context->next_dynamic_user_library_id++;
            status = VX_SUCCESS;
        }
    }
    return status;
}

static vx_target_t* findTargetByString(vx_context_t* context, const char* target_string)
{
    vx_uint32 t = 0;
    vx_target_t* target = NULL;

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

        for (t = 0; t < context->num_targets; t++)
        {
            vx_uint32 rt = context->priority_targets[t];
            vx_target_t* curr_target = &context->targets[rt];
            if (ownMatchTargetNameWithString(curr_target->name, target_lower_string) == vx_true_e)
            {
                target = curr_target;
            }
        }

        free(target_lower_string);
    }

    return target;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetImmediateModeTarget(vx_context context, vx_enum target_enum, const char* target_string)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (ownIsValidContext(context) == vx_true_e)
    {
        vx_target_t* target = NULL;
        switch (target_enum)
        {
            case VX_TARGET_ANY:
                context->imm_target_enum = VX_TARGET_ANY;
                memset(context->imm_target_string, 0, sizeof(context->imm_target_string));
                status = VX_SUCCESS;
                break;

            case VX_TARGET_STRING:
                target = findTargetByString(context, target_string);
                if (target != NULL) /* target was found */
                {
                    context->imm_target_enum = VX_TARGET_STRING;
                    strncpy(context->imm_target_string, target_string, sizeof(context->imm_target_string));
                    context->imm_target_string[sizeof(context->imm_target_string) - 1] = '\0';
                    status = VX_SUCCESS;
                }
                else /* target was not found */
                {
                    status = VX_ERROR_NOT_SUPPORTED;
                }
                break;

            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }
    return status;
}

#ifdef OPENVX_USE_OPENCL_INTEROP
VX_API_ENTRY vx_context VX_API_CALL vxCreateContextFromCL(cl_context opencl_context, cl_command_queue opencl_command_queue)
{
    vx_context context = vxCreateContext();
    context->opencl_context = opencl_context;
    context->opencl_command_queue = opencl_command_queue;
    return context;
}
#endif
