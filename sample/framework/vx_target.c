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
#include "vx_target.h"


void ownPrintTarget(vx_target target, vx_uint32 index)
{
    if (target)
    {
        ownPrintReference(&target->base);
        VX_PRINT(VX_ZONE_TARGET, "Target[%u]=>%s\n", index, target->name);
    }
}

vx_status ownUnloadTarget(vx_context_t *context, vx_uint32 index, vx_bool unload_module)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_target target = &context->targets[index];
    vx_target *targ = &target;
    if (index < VX_INT_MAX_NUM_TARGETS)
    {
        memset(&context->targets[index].funcs, 0xFE, sizeof(vx_target_funcs_t));
        if (ownDecrementReference(&context->targets[index].base, VX_INTERNAL) == 0)
        {
            /* The ownReleaseReferenceInt() below errors out if the internal index is 0 */
            ownIncrementReference(&context->targets[index].base, VX_INTERNAL);
            if (unload_module)
            {
                ownUnloadModule(context->targets[index].module.handle);
                context->targets[index].module.handle = VX_MODULE_INIT;
            }

            memset(context->targets[index].module.name, 0, sizeof(context->targets[index].module.name));
            ownReleaseReferenceInt((vx_reference *)targ, VX_TYPE_TARGET, VX_INTERNAL, NULL);
        }
        status = VX_SUCCESS;
    }
    return status;
}

vx_status ownLoadTarget(vx_context context, vx_char *name)
{
    vx_status status = VX_FAILURE;
    vx_uint32 index = 0u;

    for (index = 0; index < VX_INT_MAX_NUM_TARGETS; index++)
    {
        if (context->targets[index].module.handle == VX_MODULE_INIT)
        {
            break;
        }
    }
    if (index < VX_INT_MAX_NUM_TARGETS)
    {
        vx_char module[VX_INT_MAX_PATH];

#if defined(WIN32)
        _snprintf(module, VX_INT_MAX_PATH, VX_MODULE_NAME("%s"), (name?name:"openvx-target"));
        module[VX_INT_MAX_PATH-1] = 0; // for MSVC which is not C99 compliant
#else
        snprintf(module, VX_INT_MAX_PATH, VX_MODULE_NAME("%s"), (name?name:"openvx-target"));
#endif
        context->targets[index].module.handle = ownLoadModule(module);
        if (context->targets[index].module.handle)
        {
            ownInitReference(&context->targets[index].base, context, VX_TYPE_TARGET, &context->base);
            ownIncrementReference(&context->targets[index].base, VX_INTERNAL);
            if (ownAddReference(context, &context->targets[index].base) == vx_true_e)
            {
                context->targets[index].funcs.init     = (vx_target_init_f)    ownGetSymbol(context->targets[index].module.handle, "vxTargetInit");
                context->targets[index].funcs.deinit   = (vx_target_deinit_f)  ownGetSymbol(context->targets[index].module.handle, "vxTargetDeinit");
                context->targets[index].funcs.supports = (vx_target_supports_f)ownGetSymbol(context->targets[index].module.handle, "vxTargetSupports");
                context->targets[index].funcs.process  = (vx_target_process_f) ownGetSymbol(context->targets[index].module.handle, "vxTargetProcess");
                context->targets[index].funcs.verify   = (vx_target_verify_f)  ownGetSymbol(context->targets[index].module.handle, "vxTargetVerify");
                context->targets[index].funcs.addkernel= (vx_target_addkernel_f)ownGetSymbol(context->targets[index].module.handle, "vxTargetAddKernel");
#ifdef OPENVX_KHR_TILING
                context->targets[index].funcs.addtilingkernel = (vx_target_addtilingkernel_f)ownGetSymbol(context->targets[index].module.handle, "vxTargetAddTilingKernel");
#endif
                if (context->targets[index].funcs.init &&
                    context->targets[index].funcs.deinit &&
                    context->targets[index].funcs.supports &&
                    context->targets[index].funcs.process &&
                    context->targets[index].funcs.verify &&
                    context->targets[index].funcs.addkernel)
                    /* tiling kernel function can be NULL */
                {
                    VX_PRINT(VX_ZONE_TARGET, "Loaded target %s\n", module);
                    status = VX_SUCCESS;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Failed to load target %s due to missing symbols!\n", module);
                    ownUnloadTarget(context, index, vx_true_e);
                    status = VX_ERROR_NO_RESOURCES;
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Failed to load target %s\n", module);
                status = VX_ERROR_NO_RESOURCES;
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_WARNING, "Failed to load target %s\n", module);
            status = VX_ERROR_NO_RESOURCES;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "No free targets remain!\n");
    }
    return status;
}

vx_uint32 vxFindTargetIndex(vx_target target)
{
    vx_uint32 t = 0u;
    for (t = 0u; t < target->base.context->num_kernels; t++)
    {
        if (target == &target->base.context->targets[t])
        {
            break;
        }
    }
    return t;
}

vx_status ownInitializeTarget(vx_target target,
                             vx_kernel_description_t *kernels[],
                             vx_uint32 numkernels)
{
    vx_uint32 k = 0u;
    vx_status status = VX_FAILURE;
    for (k = 0u; k < numkernels; k++)
    {
        status = ownInitializeKernel(target->base.context, &target->kernels[k],
                                   kernels[k]->enumeration,
                                   kernels[k]->function,
                                   kernels[k]->name,
                                   kernels[k]->parameters,
                                   kernels[k]->numParams,
                                   kernels[k]->validate,
                                   kernels[k]->input_validate,
                                   kernels[k]->output_validate,
                                   kernels[k]->initialize,
                                   kernels[k]->deinitialize);
        VX_PRINT(VX_ZONE_KERNEL, "Initialized Kernel %s, %d\n", kernels[k]->name, status);
        if (status != VX_SUCCESS) {
            break;
        }
        target->num_kernels++;
        status = vxFinalizeKernel(&target->kernels[k]);
        if (status != VX_SUCCESS) {
            break;
        }
    }
    return status;
}

vx_status ownDeinitializeTarget(vx_target target)
{
    vx_uint32 k = 0u;
    vx_status status = VX_SUCCESS;
    vx_kernel kernel = NULL;

    for (k = 0u; k < target->num_kernels; k++)
    {
        kernel = &(target->kernels[k]);

        if ((kernel->enabled != vx_false_e) ||
            (kernel->enumeration != VX_KERNEL_INVALID))
        {
            kernel->enabled = vx_false_e;

            if (ownIsKernelUnique(&target->kernels[k]) == vx_true_e)
            {
                target->base.context->num_unique_kernels--;
            }

            if( ownDeinitializeKernel(&kernel) != VX_SUCCESS )
            {
                status = VX_FAILURE;
            }
        }
    }

    target->base.context->num_kernels -= target->num_kernels;
    target->num_kernels = 0;

    return status;
}

static const char* reverse_strstr(const char* string, const char* substr)
{
    const char* last = NULL;
    const char* cur = string;
    do {
        cur = (const char*) strstr(cur, substr);
        if (cur != NULL)
        {
            last = cur;
            cur = cur+1;
        }
    } while (cur != NULL);
    return last;
}

vx_bool ownMatchTargetNameWithString(const char* target_name, const char* target_string)
{
    /* 1. find latest occurrence of target_string in target_name;
       2. match only the cases: target_name == "[smth.]<target_string>[.smth]"
     */
    const char dot = '.';
    vx_bool match = vx_false_e;
    const char* ptr = reverse_strstr(target_name, target_string);
    if (ptr != NULL)
    {
        vx_size name_len = strlen(target_name);
        vx_size string_len = strlen(target_string);
        vx_size begin = (vx_size)(ptr - target_name);
        vx_size end = begin + string_len;
        if ((!(begin > 0) || ((begin > 0) && (target_name[begin-1] == dot))) ||
            (!(end < name_len) || ((end < name_len) && (target_name[end] == dot))))
        {
            match = vx_true_e;
        }
    }
    return match;
}

/******************************************************************************/
/* PUBLIC API */
/******************************************************************************/


