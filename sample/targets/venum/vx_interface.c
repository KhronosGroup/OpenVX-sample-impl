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

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <vx_interface.h>

vx_status VX_CALLBACK vxTilingKernel(vx_node node, const vx_reference parameters[], vx_uint32 num);

static const vx_char name[VX_MAX_TARGET_NAME] = "khronos.venum";

/*! \brief Declares the list of all supported base kernels.
 * \ingroup group_implementation
 * \note This is the list of all supported base kernels! It must at least
 * match the OpenVX 1.0 Specification.
 */
static vx_kernel_description_t *target_kernels[] =
{
    &absdiff_kernel,
    &not_kernel,
    &median3x3_kernel,
    &erode3x3_kernel,
    &dilate3x3_kernel,
    &min_kernel,
    &subtract_kernel,
    &threshold_kernel,
    &integral_image_kernel,
    &mean_stddev_kernel,
    &phase_kernel,
    &equalize_hist_kernel,
    &nonlinearfilter_kernel,
    &match_template_kernel,
    &box3x3_kernel,
    &colorconvert_kernel,
    &laplacian_reconstruct_kernel,
    &sobel3x3_kernel,
    &convertdepth_kernel,
    &warp_affine_kernel,
    &warp_perspective_kernel,
    &copy_kernel,
    &gaussian_pyramid_kernel,
    &scale_image_kernel,
    &laplacian_pyramid_kernel,
    &lbp_kernel,
    &max_kernel,
    &add_kernel,
    &channelcombine_kernel,
    &convolution_kernel,
    &remap_kernel,
    &fast9_kernel,
    &minmaxloc_kernel,
    &lut_kernel,
    &tensor_multiply_kernel,
    &tensor_add_kernel,
    &tensor_subtract_kernel,
    &bilateral_filter_kernel,
    &or_kernel,
    &and_kernel,
    &xor_kernel,
    &gaussian3x3_kernel,
    &multiply_kernel,
    &channelextract_kernel,
    &histogram_kernel,
    &magnitude_kernel,
    &optpyrlk_kernel,
    &tensor_transpose_kernel,
    &tensor_matrix_multiply_kernel,
    &halfscale_gaussian_kernel,
    &hogfeatures_kernel,
    &hogcells_kernel, 
    &nonmaxsuppression_kernel,
    &houghlinesp_kernel,
    &tensor_lut_kernel,
    &harris_kernel,
	&weightedaverage_kernel,
};

/*! \brief Declares the number of base supported kernels.
 * \ingroup group_implementation
 */
static vx_uint32 num_target_kernels = dimof(target_kernels);

/******************************************************************************/
/* EXPORTED FUNCTIONS */
/******************************************************************************/

vx_status vxTargetInit(vx_target target)
{
    if (target)
    {
        strncpy(target->name, name, VX_MAX_TARGET_NAME);
#if defined(EXPERIMENTAL_USE_VENUM)
        target->priority = VX_TARGET_PRIORITY_VENUM;
#endif
    }
    return ownInitializeTarget(target, target_kernels, num_target_kernels);
}

vx_status vxTargetDeinit(vx_target target)
{
    return ownDeinitializeTarget(target);
}

vx_status vxTargetSupports(vx_target target,
                           vx_char targetName[VX_MAX_TARGET_NAME],
                           vx_char kernelName[VX_MAX_KERNEL_NAME],
#if defined(EXPERIMENTAL_USE_VARIANTS)
                           vx_char variantName[VX_MAX_VARIANT_NAME],
#endif
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
#if defined(EXPERIMENTAL_USE_VARIANTS)
            vx_char *variant;
#endif

            strncpy(targetKernelName, target->kernels[k].name, VX_MAX_KERNEL_NAME);
            kernel = strtok(targetKernelName, ":");
            if (kernel == NULL)
                kernel = def;

#if defined(EXPERIMENTAL_USE_VARIANTS)
            variant = strtok(NULL, ":");

            if (variant == NULL)
                variant = def;
#endif
            if (strncmp(kernelName, kernel, VX_MAX_KERNEL_NAME) == 0
#if defined(EXPERIMENTAL_USE_VARIANTS)
                && strncmp(variantName, variant, VX_MAX_VARIANT_NAME) == 0
#endif
                )
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
                            vx_tiling_kernel_f flexible_func_ptr,
                            vx_tiling_kernel_f fast_func_ptr,
                            vx_uint32 numParams,
                            vx_kernel_input_validate_f input,
                            vx_kernel_output_validate_f output)
{
    vx_uint32 k = 0u;
    vx_kernel_t *kernel = NULL;
    for (k = 0; k < VX_INT_MAX_KERNELS; k++)
    {
        kernel = &(target->kernels[k]);
        if (kernel->enabled == vx_false_e)
        {
            kernel->tiling_function = fast_func_ptr;
            ownInitializeKernel(target->base.context,
                               kernel,
                               enumeration, vxTilingKernel, name,
                               NULL, numParams,
                               NULL, input, output, NULL, NULL);
            VX_PRINT(VX_ZONE_KERNEL, "Reserving %s Kernel[%u] for %s\n", target->name, k, kernel->name);
            target->num_kernels++;
            break;
        }
        kernel = NULL;
    }
    return (vx_kernel)kernel;
}

static vx_status vxGetPatchToTile(vx_image image, vx_rectangle_t *rect, vx_tile_t *tile)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 p = 0;
    vx_image_t *img = (vx_image_t *)image;
    for (p = 0; p < img->planes; p++)
    {
        tile->base[p] = NULL;
        status = vxAccessImagePatch(image, rect, 0, &tile->addr[p], (void **)&tile->base[p], VX_READ_AND_WRITE);
    }
    return status;
}

static vx_status vxSetTileToPatch(vx_image image, vx_rectangle_t *rect, vx_tile_t *tile)
{
    vx_image_t *img = (vx_image_t *)image;
    vx_uint32 p = 0;
    vx_status status = VX_SUCCESS;;
    for (p = 0; p < img->planes; p++)
    {
        status = vxCommitImagePatch(image, rect, 0, &tile->addr[p], tile->base[p]);
    }
    return status;
}

vx_status VX_CALLBACK vxTilingKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_image images[VX_INT_MAX_PARAMS];
    vx_uint32 ty = 0u, tx = 0u, p = 0u;
    vx_rectangle_t rect;
    vx_tile_t tiles[VX_INT_MAX_PARAMS];
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

    /* Do the following:
     * \arg find out each parameters direction
     * \arg assign each image from the parameters
     * \arg assign the block/neighborhood info
     */
    for (p = 0u; p < num; p++)
    {
        vx_parameter param = vxGetParameterByIndex(node, p);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vxQueryParameter(param, VX_PARAMETER_DIRECTION, &dirs[p], sizeof(dirs[p]));
            vxQueryParameter(param, VX_PARAMETER_TYPE, &types[p], sizeof(types[p]));
            vxReleaseParameter(&param);
        }
        //printf("Tiling Kernel Parameter[%u] dir:%d type:0%08x\n", p, dirs[p], types[p]);
        if (types[p] == VX_TYPE_IMAGE)
        {
            vxQueryNode(node, VX_NODE_OUTPUT_TILE_BLOCK_SIZE, &tiles[p].tile_block, sizeof(vx_tile_block_size_t));
            vxQueryNode(node, VX_NODE_INPUT_NEIGHBORHOOD, &tiles[p].neighborhood, sizeof(vx_neighborhood_size_t));
            ownPrintImage((vx_image_t *)parameters[p]);
            images[p] = (vx_image)parameters[p];
            vxQueryImage(images[p], VX_IMAGE_WIDTH, &tiles[p].image.width, sizeof(vx_uint32));
            vxQueryImage(images[p], VX_IMAGE_HEIGHT, &tiles[p].image.height, sizeof(vx_uint32));
            vxQueryImage(images[p], VX_IMAGE_FORMAT, &tiles[p].image.format, sizeof(vx_df_image));
            vxQueryImage(images[p], VX_IMAGE_SPACE, &tiles[p].image.space, sizeof(vx_enum));
            vxQueryImage(images[p], VX_IMAGE_RANGE, &tiles[p].image.range, sizeof(vx_enum));
            params[p] = &tiles[p];
            if ((dirs[p] == VX_OUTPUT) && (index == UINT32_MAX))
            {
                index = p;
                //printf("Using index %u as coordinate basis\n", index);
            }
        }
        else if (types[p] == VX_TYPE_SCALAR)
        {
            vxCopyScalar((vx_scalar)parameters[p], (void *)&scalars[p], VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            params[p] = &scalars[p];
        }
#if defined(OPENVX_TILING_1_1)
        /*! \todo add addition data types here */
#endif
    }

    /* choose the index of the first output image to based the tiling on */
    status |= vxQueryImage(images[index], VX_IMAGE_WIDTH, &width, sizeof(width));
    status |= vxQueryImage(images[index], VX_IMAGE_HEIGHT, &height, sizeof(height));
    status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
    status |= vxQueryNode(node, VX_NODE_INPUT_NEIGHBORHOOD, &nbhd, sizeof(nbhd));
    status |= vxQueryNode(node, VX_NODE_TILE_MEMORY_SIZE, &size, sizeof(size));

#if 0
    tile_size_y = (height - (nbhd.y[1] + abs(nbhd.y[0]))) / block_multiple;
    tile_size_x = (width - (nbhd.x[1] + abs(nbhd.x[0])));
#else
    tile_size_y = height / block_multiple;
    tile_size_x = width;
#endif

    if ((borders.mode != VX_BORDER_UNDEFINED) &&
        (borders.mode != VX_BORDER_MODE_SELF))
    {
        return VX_ERROR_NOT_SUPPORTED;
    }

    status = VX_SUCCESS;
#if 0
    for (ty = abs(nbhd.y[0]); (ty < (height - nbhd.y[1])) && (status == VX_SUCCESS); ty += tile_size_y)
    {
        for (tx = abs(nbhd.x[0]); tx < (width - nbhd.x[1]); tx += tile_size_x)
        {
#else
    for (ty = 0u; (ty < height) && (status == VX_SUCCESS); ty += tile_size_y)
    {
        for (tx = 0u; tx < width; tx += tile_size_x)
        {
#endif
            rect.start_x = tx;
            rect.start_y = ty;
            rect.end_x = tx+tile_size_x;
            rect.end_y = ty+tile_size_y;
            for (p = 0u; p < num; p++)
            {
                if (types[p] == VX_TYPE_IMAGE)
                {
                    tiles[p].tile_x = tx;
                    tiles[p].tile_y = ty;
                    status |= vxGetPatchToTile(images[p], &rect, &tiles[p]);
                }
            }
            if (status == VX_SUCCESS)
            {
                //printf("Calling Tile{%u,%u} with %s\n", tx, ty, ((vx_node_t *)node)->kernel->name);
                tile_memory = ((vx_node_t *)node)->attributes.tileDataPtr;
                ((vx_node_t *)node)->kernel->tiling_function(params, tile_memory, size);
            }
            else
            {
                //printf("Failed to get tile {%u, %u} (status = %d)\n", tx, ty, status);
            }
            for (p = 0u; p < num; p++)
            {
                if (types[p] == VX_TYPE_IMAGE)
                {
                    if (dirs[p] == VX_INPUT)
                    {
                        status |= vxSetTileToPatch(images[p], 0, &tiles[p]);
                    }
                    else
                    {
                        status |= vxSetTileToPatch(images[p], &rect, &tiles[p]);
                    }
                }
            }
            if (status != VX_SUCCESS)
            {
                break;
            }
        }
    }
    //printf("Tiling Kernel returning = %d\n", status);
    return status;
}
#endif

