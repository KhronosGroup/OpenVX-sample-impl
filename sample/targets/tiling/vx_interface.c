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

/*!
 * \file
 * \brief The C-Model Target Interface
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include "vx_internal.h"
#include <vx_interface.h>
#include <tiling.h>

vx_status VX_CALLBACK vxTilingKernel(vx_node node, const vx_reference parameters[], vx_uint32 num);

static const vx_char name[VX_MAX_TARGET_NAME] = "khronos.tiling";

vx_tiling_kernel_t *tiling_kernels[] =
{
    &box_3x3_kernels,
    &phase_kernel,
    &And_kernel,
    &Or_kernel,
    &Xor_kernel,
    &Not_kernel,
    &threshold_kernel,
    &colorconvert_kernel,
    &Multiply_kernel,
    &nonlinearfilter_kernel,
    &Magnitude_kernel,
    &erode3x3_kernel,
    &dilate3x3_kernel,
    &median3x3_kernel,
    &sobel3x3_kernel,
    &Max_kernel,
    &Min_kernel,
    &gaussian3x3_kernel,
    &add_kernel,
    &subtract_kernel,
    &convertdepth_kernel,
    &warp_affine_kernel,
    &warp_perspective_kernel,
    &weightedaverage_kernel,
    &absdiff_kernel,
    &integral_image_kernel,
    &remap_kernel,
    &convolution_kernel,
    &hogfeatures_kernel,
    &fast9_kernel,
    &lbp_kernel,
    &scale_image_kernel,
    &lut_kernel,
    &channelcombine_kernel,
    &halfscale_gaussian_kernel,
    &nonmaxsuppression_kernel,
    &hogcells_kernel,
    &houghlinesp_kernel,
};

static vx_bool ownIsValidContext_tiling(vx_context context)
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

static vx_size strnindex(const vx_char *str, vx_char c, vx_size limit)
{
    vx_size index = 0;
    while (index < limit && *str != c)
    {
        if(!*str)
        {
            index = limit;
            break;
        }
        str++;
        index++;
	}
    return index;
}

VX_API_ENTRY vx_kernel VX_API_CALL vxAddTilingKernelEx(vx_context c,
                            vx_char name[VX_MAX_KERNEL_NAME],
                            vx_enum enumeration,
                            vx_kernel_f function,
                            vx_tiling_kernel_f flexible_func_ptr,
                            vx_tiling_kernel_f fast_func_ptr,
                            vx_uint32 num_params,
                            vx_kernel_validate_f validate,
                            vx_kernel_input_validate_f input,
                            vx_kernel_output_validate_f output,
                            vx_kernel_initialize_f initialize,
                            vx_kernel_deinitialize_f deinitialize)
{
    vx_context_t *context = (vx_context_t *)c;
    vx_kernel kernel = 0;
    vx_uint32 t = 0;
    vx_size index = 0;
    vx_target_t *target = NULL;
    vx_char targetName[VX_MAX_TARGET_NAME];

    if (ownIsValidContext_tiling(context) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid Context\n");
        return (vx_kernel)NULL;
    }
    if  ( ((validate == NULL) && (input == NULL || output == NULL)) ||       
        num_params > VX_INT_MAX_PARAMS || num_params == 0 ||
        name == NULL ||
        strncmp(name, "",  VX_MAX_KERNEL_NAME) == 0)
        /* initialize and de-initialize can be NULL */
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid Parameters!\n");
        vxAddLogEntry((vx_reference)c, VX_ERROR_INVALID_PARAMETERS, "Invalid Parameters supplied to vxAddKernel\n");
        return (vx_kernel)NULL;
    }

    /* find target to assign this to */
    index = strnindex(name, ':', VX_MAX_TARGET_NAME);
    if (index == VX_MAX_TARGET_NAME)
    {
        strcpy(targetName,"khronos.tiling");
    }
    else
    {
        strncpy(targetName, name, index);
    }
    VX_PRINT(VX_ZONE_KERNEL, "Deduced Name as %s\n", targetName);
    for (t = 0u; t < context->num_targets; t++)
    {
        target = &context->targets[t];
        if (strncmp(targetName,target->name, VX_MAX_TARGET_NAME) == 0)
        {
            break;
        }
        target = NULL;
    }
    if (target && target->funcs.addtilingkernel)
    {
        kernel = target->funcs.addtilingkernel(target, name, enumeration, function,
                                         flexible_func_ptr, fast_func_ptr, num_params, validate,
                                         input, output, initialize, deinitialize);
        VX_PRINT(VX_ZONE_KERNEL,"Added Kernel %s to Target %s ("VX_FMT_REF")\n", name, target->name, kernel);
    }
    else
    {
        vxAddLogEntry((vx_reference)c, VX_ERROR_NO_RESOURCES, "No target named %s exists!\n", targetName);
    }
    return (vx_kernel)kernel;
}

/*! \brief The Entry point into a user defined kernel module */
vx_status VX_API_CALL vxPublishKernels(vx_context context)
{
    // tag::publish_function[]
    vx_status status = VX_SUCCESS;
    vx_uint32 k = 0;
    for (k = 0; k < dimof(tiling_kernels); k++)
    {
        if (k == 34)
		{
			int aa = 0;
		}
        vx_kernel kernel = vxAddTilingKernelEx(context,
            tiling_kernels[k]->name,
            tiling_kernels[k]->enumeration,
            tiling_kernels[k]->function,
            tiling_kernels[k]->flexible_function,
            tiling_kernels[k]->fast_function,
            tiling_kernels[k]->num_params,            
            tiling_kernels[k]->validate,
            tiling_kernels[k]->input_validator,
            tiling_kernels[k]->output_validator,
            tiling_kernels[k]->initialize,
            tiling_kernels[k]->deinitialize);
        if (kernel)
        {
            vx_uint32 p = 0;
            for (p = 0; p < tiling_kernels[k]->num_params; p++)
            {
                status |= vxAddParameterToKernel(kernel, p,
                    tiling_kernels[k]->parameters[p].direction,
                    tiling_kernels[k]->parameters[p].data_type,
                    tiling_kernels[k]->parameters[p].state);
            }
            status |= vxSetKernelAttribute(kernel, VX_KERNEL_INPUT_NEIGHBORHOOD,
                &tiling_kernels[k]->nbhd, sizeof(vx_neighborhood_size_t));
            status |= vxSetKernelAttribute(kernel, VX_KERNEL_OUTPUT_TILE_BLOCK_SIZE,
                &tiling_kernels[k]->block, sizeof(vx_tile_block_size_t));
            status |= vxSetKernelAttribute(kernel, VX_KERNEL_BORDER,
                &tiling_kernels[k]->border, sizeof(vx_border_t));
            if (status != VX_SUCCESS)
            {
                vxRemoveKernel(kernel);
            }
            else
            {
                status = vxFinalizeKernel(kernel);
            }
            if (status != VX_SUCCESS)
            {
                printf("Failed to publish kernel %s\n", tiling_kernels[k]->name);
                break;
            }
        }
    }
    // end::publish_function[]
    return status;
}


/*VX_API_ENTRY*/ vx_status VX_API_CALL vxUnpublishKernels(vx_context context)
{
    vx_status status = VX_FAILURE;

    vx_uint32 k = 0;
    for (k = 0; k < dimof(tiling_kernels); k++)
    {
        vx_kernel kernel = vxGetKernelByName(context, tiling_kernels[k]->name);
        kernel->user_kernel = 1;
        vx_kernel kernelcpy = kernel;

        if (kernel)
        {
            status = vxReleaseKernel(&kernelcpy);
            if (status != VX_SUCCESS)
            {
                vxAddLogEntry((vx_reference)context, status, "Failed to release kernel[%u]=%s\n", k, tiling_kernels[k]->name);
            }
            else
            {
                kernelcpy = kernel;
                status = vxRemoveKernel(kernelcpy);
                if (status != VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)context, status, "Failed to remove kernel[%u]=%s\n", k, tiling_kernels[k]->name);
                }
            }
        }
        else
        {
            vxAddLogEntry((vx_reference)context, status, "Failed to get added kernel %s\n", tiling_kernels[k]->name);
        }
    }

    return status;
}

vx_status vxTargetInit(vx_target target)
{
    if (target)
    {
        strncpy(target->name, name, VX_MAX_TARGET_NAME);
        target->priority = VX_TARGET_PRIORITY_TILING;
    }
    return vxPublishKernels(target->base.context);
}

vx_status vxTargetDeinit(vx_target target)
{
    return vxUnpublishKernels(target->base.context);
}

vx_status vxTargetSupports(vx_target target,
                           vx_char targetName[VX_MAX_TARGET_NAME],
                           vx_char kernelName[VX_MAX_KERNEL_NAME],
                           vx_uint32 *pIndex)
{
    vx_status status = VX_ERROR_NOT_SUPPORTED;
    if (strncmp(targetName, name, VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "default", VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "power", VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "performance", VX_MAX_TARGET_NAME) == 0)
    {
        vx_uint32 k = 0u;
        for (k = 0u; k < VX_INT_MAX_KERNELS; k++)
        {
            vx_char targetKernelName[VX_MAX_KERNEL_NAME];
            vx_char *kernel;
            vx_char def[8] = "default";

            strncpy(targetKernelName, target->kernels[k].name, VX_MAX_KERNEL_NAME);
            kernel = strtok(targetKernelName, ":");
            if (kernel == NULL)
                kernel = def;

            if (strncmp(kernelName, kernel, VX_MAX_KERNEL_NAME) == 0)
            {
                status = VX_SUCCESS;
                if (pIndex) *pIndex = k;
                break;
            }
        }
    }
    return status;
}

vx_action vxTargetProcess(vx_target target, vx_node_t *nodes[], vx_size startIndex, vx_size numNodes)
{
    vx_action action = VX_ACTION_CONTINUE;
    vx_status status = VX_SUCCESS;
    vx_size n = 0;
    for (n = startIndex; (n < (startIndex + numNodes)) && (action == VX_ACTION_CONTINUE); n++)
    {
        vx_context context = vxGetContext((vx_reference)nodes[n]);
        VX_PRINT(VX_ZONE_GRAPH, "Executing Kernel %s:%d in Nodes[%u] on target %s\n",
            nodes[n]->kernel->name,
            nodes[n]->kernel->enumeration,
            n,
            nodes[n]->base.context->targets[nodes[n]->affinity].name);

        if (context->perf_enabled)
            ownStartCapture(&nodes[n]->perf);

        if (nodes[n]->is_replicated == vx_true_e)
        {
            vx_size num_replicas = 0;
            vx_uint32 param;
            vx_uint32 num_parameters = nodes[n]->kernel->signature.num_parameters;
            vx_reference parameters[VX_INT_MAX_PARAMS] = { NULL };

            for (param = 0; param < num_parameters; ++param)
            {
                if (nodes[n]->replicated_flags[param] == vx_true_e)
                {
                    vx_size numItems = 0;
                    if ((nodes[n]->parameters[param])->scope->type == VX_TYPE_PYRAMID)
                    {
                        vx_pyramid pyr = (vx_pyramid)(nodes[n]->parameters[param])->scope;
                        numItems = pyr->numLevels;
                    }
                    else if ((nodes[n]->parameters[param])->scope->type == VX_TYPE_OBJECT_ARRAY)
                    {
                        vx_object_array arr = (vx_object_array)(nodes[n]->parameters[param])->scope;
                        numItems = arr->num_items;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                        break;
                    }

                    if (num_replicas == 0)
                        num_replicas = numItems;
                    else if (numItems != num_replicas)
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                        break;
                    }
                }
                else
                {
                    parameters[param] = nodes[n]->parameters[param];
                }
            }

            if (status == VX_SUCCESS)
            {
                vx_size replica;
                for (replica = 0; replica < num_replicas; ++replica)
                {
                    for (param = 0; param < num_parameters; ++param)
                    {
                        if (nodes[n]->replicated_flags[param] == vx_true_e)
                        {
                            if ((nodes[n]->parameters[param])->scope->type == VX_TYPE_PYRAMID)
                            {
                                vx_pyramid pyr = (vx_pyramid)(nodes[n]->parameters[param])->scope;
                                parameters[param] = (vx_reference)pyr->levels[replica];
                            }
                            else if ((nodes[n]->parameters[param])->scope->type == VX_TYPE_OBJECT_ARRAY)
                            {
                                vx_object_array arr = (vx_object_array)(nodes[n]->parameters[param])->scope;
                                parameters[param] = (vx_reference)arr->items[replica];
                            }
                        }
                    }

                    status = nodes[n]->kernel->function((vx_node)nodes[n],
                        parameters,
                        num_parameters);
                }
            }
        }
        else
        {
            status = nodes[n]->kernel->function((vx_node)nodes[n],
                (vx_reference *)nodes[n]->parameters,
                nodes[n]->kernel->signature.num_parameters);
        }

        nodes[n]->executed = vx_true_e;
        nodes[n]->status = status;

        if (context->perf_enabled)
            ownStopCapture(&nodes[n]->perf);

        VX_PRINT(VX_ZONE_GRAPH, "kernel %s returned %d\n", nodes[n]->kernel->name, status);

        if (status == VX_SUCCESS)
        {
            /* call the callback if it is attached */
            if (nodes[n]->callback)
            {
                action = nodes[n]->callback((vx_node)nodes[n]);
                VX_PRINT(VX_ZONE_GRAPH, "callback returned action %d\n", action);
            }
        }
        else
        {
            action = VX_ACTION_ABANDON;
            VX_PRINT(VX_ZONE_ERROR, "Abandoning Graph due to error (%d)!\n", status);
        }
    }
    return action;
}

vx_status vxTargetVerify(vx_target target, vx_node_t *node)
{
    vx_status status = VX_SUCCESS;
    return status;
}

vx_kernel vxTargetAddKernel(vx_target target,
                            vx_char name[VX_MAX_KERNEL_NAME],
                            vx_enum enumeration,
                            vx_kernel_f func_ptr,
                            vx_uint32 numParams,
                            vx_kernel_validate_f validate,
                            vx_kernel_input_validate_f input,
                            vx_kernel_output_validate_f output,
                            vx_kernel_initialize_f initialize,
                            vx_kernel_deinitialize_f deinitialize)
{
    vx_uint32 k = 0u;
    vx_kernel_t *kernel = NULL;
    // ownSemWait(&target->base.lock);
    for (k = 0; k < VX_INT_MAX_KERNELS; k++)
    {
        kernel = &(target->kernels[k]);
        if (kernel->enabled == vx_false_e)
        {
            ownInitializeKernel(target->base.context,
                               kernel,
                               enumeration, func_ptr, name,
                               NULL, numParams,
                               validate, input, output, initialize, deinitialize);
            VX_PRINT(VX_ZONE_KERNEL, "Reserving %s Kernel[%u] for %s\n", target->name, k, kernel->name);
            target->num_kernels++;
            break;
        }
        kernel = NULL;
    }
    // ownSemPost(&target->base.lock);
    return (vx_kernel)kernel;
}

#ifdef OPENVX_KHR_TILING
vx_kernel vxTargetAddTilingKernel(vx_target target,
                            vx_char name[VX_MAX_KERNEL_NAME],
                            vx_enum enumeration,
                            vx_kernel_f function,
                            vx_tiling_kernel_f flexible_func_ptr,
                            vx_tiling_kernel_f fast_func_ptr,
                            vx_uint32 numParams,
                            vx_kernel_validate_f validate,
                            vx_kernel_input_validate_f input,
                            vx_kernel_output_validate_f output,
                            vx_kernel_initialize_f initialize,
                            vx_kernel_deinitialize_f deinitialize)
{
    vx_uint32 k = 0u;
    vx_kernel_t *kernel = NULL;
    for (k = 0; k < VX_INT_MAX_KERNELS; k++)
    {
        kernel = &(target->kernels[k]);
        if (kernel->enabled == vx_false_e)
        {
            kernel->tilingfast_function = fast_func_ptr;
            kernel->tilingflexible_function = flexible_func_ptr;

            if (function == NULL)
            {
                ownInitializeKernel(target->base.context,
                                   kernel,
                                   enumeration, vxTilingKernel, name,
                                   NULL, numParams,
                                   validate, input, output, initialize, deinitialize);
            }
            else //Kernel with more than one node like HalfScaleGaussian
            {
                ownInitializeKernel(target->base.context,
                                   kernel,
                                   enumeration, function, name,
                                   NULL, numParams,
                                   validate, input, output, initialize, deinitialize);
            }
            VX_PRINT(VX_ZONE_KERNEL, "Reserving %s Kernel[%u] for %s\n", target->name, k, kernel->name);
            target->num_kernels++;
            break;
        }
        kernel = NULL;
    }
    return (vx_kernel)kernel;
}

static vx_status vxGetPatchToTile(vx_image image, vx_rectangle_t *rect, vx_tile_ex_t *tile)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 p = 0;
    vx_image_t *img = (vx_image_t *)image;

    for (p = 0; p < img->planes; p++)
    {
        tile->base[p] = NULL;
        if(image->constant == 1)
            status = vxAccessImagePatch(image, rect, p, &tile->addr[p], (void **)&tile->base[p], VX_READ_ONLY);
        else
            status = vxAccessImagePatch(image, rect, p, &tile->addr[p], (void **)&tile->base[p], VX_READ_AND_WRITE);
    }

    return status;
}

static vx_status vxSetTileToPatch(vx_image image, vx_rectangle_t *rect, vx_tile_ex_t *tile)
{
    vx_image_t *img = (vx_image_t *)image;
    vx_uint32 p = 0;
    vx_status status = VX_SUCCESS;

    for (p = 0; p < img->planes; p++)
    {
        status = vxCommitImagePatch(image, rect, p, &tile->addr[p], tile->base[p]);
    }

    return status;
}

static void* ownAllocateTensorMemory_tiling(vx_tensor tensor)
{
    vx_size total_size = ownSizeOfType(tensor->data_type);

    if (tensor->addr == NULL)
    {
        for (vx_uint32 i = 0; i < tensor->number_of_dimensions; i++)
        {
            total_size *= tensor->dimensions[i];
        }
        tensor->addr = calloc(total_size, 1);
    }
    return tensor->addr;
}

vx_status VX_CALLBACK vxTilingKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    vx_image images[VX_INT_MAX_PARAMS];
    vx_uint32 ty = 0u, tx = 0u, p = 0u;
    vx_rectangle_t rect;
    vx_tile_ex_t tiles[VX_INT_MAX_PARAMS];
    void *params[VX_INT_MAX_PARAMS] = {NULL};
    vx_enum dirs[VX_INT_MAX_PARAMS];
    vx_enum types[VX_INT_MAX_PARAMS];
    size_t scalars[VX_INT_MAX_PARAMS];
    vx_uint32 index = UINT32_MAX;
    vx_uint32 tile_size_y = 0u, tile_size_x = 0u;
    vx_uint32 block_multiple = 64;
    vx_uint32 height = 0u, width = 0u;
    vx_border_t borders = {VX_BORDER_UNDEFINED, 0};
    vx_neighborhood_size_t nbhd;
    void *tile_memory = NULL;
    vx_size size = 0;

    vx_tile_threshold_t threshold[VX_INT_MAX_PARAMS];
    vx_tile_matrix_t mask[VX_INT_MAX_PARAMS];
    vx_tile_convolution_t conv[VX_INT_MAX_PARAMS];
    vx_tensor tensor[VX_INT_MAX_PARAMS];
    vx_tile_array_t array_t[VX_INT_MAX_PARAMS];
    vx_array arrays[VX_INT_MAX_PARAMS];
    vx_scalar scalar[VX_INT_MAX_PARAMS];
    vx_remap map[VX_INT_MAX_PARAMS];

    vx_bool is_U1 = 0;
    /* Do the following:
     * \arg find out each parameters direction
     * \arg assign each image from the parameters
     * \arg assign the block/neighborhood info
     */
    status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

    for (p = 0u; p < num; p++)
    {
        vx_parameter param = vxGetParameterByIndex(node, p);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vxQueryParameter(param, VX_PARAMETER_DIRECTION, &dirs[p], sizeof(dirs[p]));
            vxQueryParameter(param, VX_PARAMETER_TYPE, &types[p], sizeof(types[p]));
            vxReleaseParameter(&param);
        }
        if (types[p] == VX_TYPE_IMAGE)
        {
            vxQueryNode(node, VX_NODE_OUTPUT_TILE_BLOCK_SIZE, &tiles[p].tile_block, sizeof(vx_tile_block_size_t));
            vxQueryNode(node, VX_NODE_INPUT_NEIGHBORHOOD, &tiles[p].neighborhood, sizeof(vx_neighborhood_size_t));
            images[p] = (vx_image)parameters[p];
            tiles[p].is_Null = 0;
            if (images[p] == NULL)
                tiles[p].is_Null = 1;
            vxQueryImage(images[p], VX_IMAGE_WIDTH, &tiles[p].image.width, sizeof(vx_uint32));
            vxQueryImage(images[p], VX_IMAGE_HEIGHT, &tiles[p].image.height, sizeof(vx_uint32));
            vxQueryImage(images[p], VX_IMAGE_FORMAT, &tiles[p].image.format, sizeof(vx_df_image));
            vxQueryImage(images[p], VX_IMAGE_SPACE, &tiles[p].image.space, sizeof(vx_enum));
            vxQueryImage(images[p], VX_IMAGE_RANGE, &tiles[p].image.range, sizeof(vx_enum));

            vx_rectangle_t rect_U1;
            vxGetValidRegionImage(images[p], &rect_U1);
            tiles[p].rect = rect_U1;
            tiles[p].is_U1 = 0;
            if (tiles[p].image.format == VX_DF_IMAGE_U1)
            {
                is_U1 = 1;
                tiles[p].is_U1 = is_U1;
            }
            tiles[p].border = borders;
            params[p] = &tiles[p];
            if ((dirs[p] == VX_OUTPUT) && (index == UINT32_MAX))
            {
                index = p;
            }
        }
        else if (types[p] == VX_TYPE_SCALAR)
        {
            scalar[p] = (vx_scalar)parameters[p];
            vxCopyScalar((vx_scalar)parameters[p], (void *)&scalars[p], VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            params[p] = &scalars[p];
        }
        else if (types[p] == VX_TYPE_THRESHOLD)
        {
            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_TYPE, &threshold[p].thresh_type, sizeof(threshold[p].thresh_type));
            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_THRESHOLD_VALUE, &threshold[p].value, sizeof(threshold[p].value));
            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_THRESHOLD_LOWER, &threshold[p].lower, sizeof(threshold[p].lower));
            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_THRESHOLD_UPPER, &threshold[p].upper, sizeof(threshold[p].upper));
            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_TRUE_VALUE, &threshold[p].true_value, sizeof(threshold[p].true_value));
            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_FALSE_VALUE, &threshold[p].false_value, sizeof(threshold[p].false_value));
            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_INPUT_FORMAT, &threshold[p].input_format, sizeof(threshold[p].input_format));

            vxQueryThreshold((vx_threshold)parameters[p], VX_THRESHOLD_OUTPUT_FORMAT, &threshold[p].output_format, sizeof(threshold[p].output_format));

            if (threshold[p].output_format == VX_DF_IMAGE_U1)
                is_U1 = 1;

            params[p] = &threshold[p];
        }
        else if (types[p] == VX_TYPE_MATRIX)
        {
            vxQueryMatrix((vx_matrix)parameters[p], VX_MATRIX_ROWS, &mask[p].rows, sizeof(mask[p].rows));
            vxQueryMatrix((vx_matrix)parameters[p], VX_MATRIX_COLUMNS, &mask[p].columns, sizeof(mask[p].columns));
            vxQueryMatrix((vx_matrix)parameters[p], VX_MATRIX_TYPE, &mask[p].data_type, sizeof(mask[p].data_type));
            vxQueryMatrix((vx_matrix)parameters[p], VX_MATRIX_ORIGIN, &mask[p].origin, sizeof(mask[p].origin));

            if ((mask[p].data_type != VX_TYPE_UINT8) || (sizeof(mask[p].m) < mask[p].rows * mask[p].columns))
                status = VX_ERROR_INVALID_PARAMETERS;

            vxCopyMatrix((vx_matrix)parameters[p], mask[p].m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            vxCopyMatrix((vx_matrix)parameters[p], mask[p].m_f32, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

            params[p] = &mask[p];
        }
        else if (types[p] == VX_TYPE_REMAP)
        {
            map[p] = (vx_remap)parameters[p];
            params[p] = &map[p];
        }
        else if (types[p] == VX_TYPE_CONVOLUTION)
        {
            vxQueryConvolution((vx_convolution)parameters[p], VX_CONVOLUTION_COLUMNS, &conv[p].conv_width, sizeof(conv[p].conv_width));
            vxQueryConvolution((vx_convolution)parameters[p], VX_CONVOLUTION_ROWS, &conv[p].conv_height, sizeof(conv[p].conv_height));
            vxQueryConvolution((vx_convolution)parameters[p], VX_CONVOLUTION_SCALE, &conv[p].scale, sizeof(conv[p].scale));

            vxCopyConvolutionCoefficients((vx_convolution)parameters[p], conv[p].conv_mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

            params[p] = &conv[p];
        }
        else if (types[p] == VX_TYPE_TENSOR)
        {
            tensor[p] = (vx_tensor)parameters[p];

            if (tensor[p]->addr == NULL)
                ownAllocateTensorMemory_tiling(tensor[p]);

            params[p] = tensor[p]->addr;
        }
        else if (types[p] == VX_TYPE_ARRAY || types[p] == VX_TYPE_LUT)
        {
            arrays[p] = (vx_array)parameters[p];

            array_t[p].ptr = ((vx_array)parameters[p])->memory.ptrs[0];
            array_t[p].capacity = ((vx_array)parameters[p])->capacity;
            array_t[p].item_size = ((vx_array)parameters[p])->item_size;
            array_t[p].item_type = ((vx_array)parameters[p])->item_type;
            array_t[p].num_items = ((vx_array)parameters[p])->num_items;
            array_t[p].offset = ((vx_array)parameters[p])->offset;

            params[p] = &array_t[p];
        }
    }

    if (index == UINT32_MAX)
        index = 0;

    /* choose the index of the first output image to based the tiling on */
    status |= vxQueryImage(images[index], VX_IMAGE_WIDTH, &width, sizeof(width));
    status |= vxQueryImage(images[index], VX_IMAGE_HEIGHT, &height, sizeof(height));
    status |= vxQueryNode(node, VX_NODE_INPUT_NEIGHBORHOOD, &nbhd, sizeof(nbhd));
    status |= vxQueryNode(node, VX_NODE_TILE_MEMORY_SIZE, &size, sizeof(size));

    tile_size_y = tiles[index].tile_block.height;
    tile_size_x = tiles[index].tile_block.width;

    status = VX_SUCCESS;

    if (node->kernel->enumeration == VX_KERNEL_WARP_AFFINE)
    {
        vx_image src_image = (vx_image)parameters[0];
        vx_rectangle_t src_rect;
        vxGetValidRegionImage(src_image, &src_rect);
        if (src_rect.start_x != 0 && borders.mode == VX_BORDER_CONSTANT)
        {
            is_U1 = 1;
        }   
    }

    if ((node->kernel->enumeration == VX_KERNEL_WARP_AFFINE || node->kernel->enumeration == VX_KERNEL_SCALE_IMAGE) && is_U1 != 0)
    {
        status = vxGetValidRegionImage(images[0], &rect);
        for (p = 0u; p < num; p++)
        {
            if (types[p] == VX_TYPE_IMAGE && images[p] != NULL && dirs[p] == VX_INPUT)
            {
                tiles[p].tile_x = 0;
                tiles[p].tile_y = 0;
                status |= vxGetPatchToTile(images[p], &rect, &tiles[p]);
            }
            else if (types[p] == VX_TYPE_IMAGE && dirs[p] == VX_OUTPUT)
            {
                rect.start_x = 0;
                rect.start_y = 0;
                rect.end_x = width;
                rect.end_y = height;
                tiles[p].tile_x = 0;
                tiles[p].tile_y = 0;
                status |= vxGetPatchToTile(images[p], &rect, &tiles[p]);
            }
        }
    }
    else if (node->kernel->enumeration == VX_KERNEL_THRESHOLD && is_U1 != 0)
    {
        status = vxGetValidRegionImage(images[0], &rect);
        for (p = 0u; p < num; p++)
        {
            if (types[p] == VX_TYPE_IMAGE && images[p] != NULL)
            {
                tiles[p].tile_x = 0;
                tiles[p].tile_y = 0;
                status |= vxGetPatchToTile(images[p], &rect, &tiles[p]);
            }
        }
    }
    else
    {
        rect.start_x = 0;
        rect.start_y = 0;
        rect.end_x = width;
        rect.end_y = height;
        for (p = 0u; p < num; p++)
        {
            if (types[p] == VX_TYPE_IMAGE && images[p] != NULL)
            {
                tiles[p].tile_x = 0;
                tiles[p].tile_y = 0;
                status |= vxGetPatchToTile(images[p], &rect, &tiles[p]);
            }
        }
    }

    vx_uint32 blkCntY = (height / tile_size_y) * tile_size_y;
    vx_uint32 blkCntX = (width / tile_size_x) * tile_size_x;

    //tiling fast function    
    if (((vx_node_t *)node)->kernel->tilingfast_function && is_U1 == 0)
    {
        for (ty = 0u; (ty < blkCntY) && (status == VX_SUCCESS); ty += tile_size_y)
        {
            for (tx = 0u; tx < blkCntX; tx += tile_size_x)
            {
                for (p = 0u; p < num; p++)
                {
                    if (types[p] == VX_TYPE_IMAGE)
                    {
                        tiles[p].tile_x = tx;
                        tiles[p].tile_y = ty;
                    }
                }
                tile_memory = ((vx_node_t *)node)->attributes.tileDataPtr;
                ((vx_node_t *)node)->kernel->tilingfast_function(params, tile_memory, size);
            }
        }
    
        if (((vx_node_t *)node)->kernel->tilingflexible_function && ((blkCntY < height) || (blkCntX < width)))
        {    
            for (p = 0u; p < num; p++)
            {
                if (types[p] == VX_TYPE_IMAGE)
                {
                    tiles[p].tile_x = tx;
                    tiles[p].tile_y = ty;
                }
            }
            tile_memory = ((vx_node_t *)node)->attributes.tileDataPtr;
            ((vx_node_t *)node)->kernel->tilingflexible_function(params, tile_memory, size);
        }
    }
    //tiling flexible function  
    else if (((vx_node_t *)node)->kernel->tilingflexible_function)
    {
        for (p = 0u; p < num; p++)
        {
            if (types[p] == VX_TYPE_IMAGE)
            {
                tiles[p].tile_x = tx;
                tiles[p].tile_y = ty;
            }
        }
        tile_memory = ((vx_node_t *)node)->attributes.tileDataPtr;
        ((vx_node_t *)node)->kernel->tilingflexible_function(params, tile_memory, size);
    }

    for (p = 0u; p < num; p++)
    {
        if (types[p] == VX_TYPE_IMAGE)
        {
            if (dirs[p] == VX_INPUT && images[p] != NULL)
            {
                status |= vxSetTileToPatch(images[p], 0, &tiles[p]);
            }
            else if (dirs[p] == VX_OUTPUT)
            {
                status |= vxSetTileToPatch(images[p], &rect, &tiles[p]);
            }
        }
        else if (types[p] == VX_TYPE_ARRAY && dirs[p] == VX_OUTPUT)
        {
            arrays[p]->memory.ptrs[0] = array_t[p].ptr;
            arrays[p]->num_items = array_t[p].num_items;
        }
        else if (types[p] == VX_TYPE_SCALAR && dirs[p] == VX_OUTPUT && scalar[p] != NULL)
        {
            scalar[p]->data.size = scalars[p];
        }
    }

    return status;
}
#endif
