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
 * \brief The Filter Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vxu.h>
#include <VX/vx_helper.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_lib_debug.h>
#include "vx_internal.h"

static vx_param_description_t harris_kernel_params[] =
{
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED}, // strength_thresh
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED}, // min_distance
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED}, // sensitivity
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED}, // gradient size
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED}, // block size
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
};

static vx_status VX_CALLBACK vxHarrisCornersKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    (void)parameters;

    vx_status status = VX_FAILURE;

    if (num == dimof(harris_kernel_params))
    {
        vx_graph subgraph = ownGetChildGraphOfNode(node);
        status = vxProcessGraph(subgraph);
    }

    return status;
}

static vx_status VX_CALLBACK vxHarrisInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if ((status == VX_SUCCESS) && (input))
            {
                vx_df_image format = 0;
                status = vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                if ((status == VX_SUCCESS) && (format == VX_DF_IMAGE_U8))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar strength_thresh = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &strength_thresh, sizeof(strength_thresh));
            if ((status == VX_SUCCESS) && (strength_thresh))
            {
                vx_enum type = 0;
                vxQueryScalar(strength_thresh, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&strength_thresh);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar min_dist = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &min_dist, sizeof(min_dist));
            if ((status == VX_SUCCESS) && (min_dist))
            {
                vx_enum type = 0;
                vxQueryScalar(min_dist, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    vx_float32 d = 0.0f;
                    status = vxCopyScalar(min_dist, &d, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((status == VX_SUCCESS) && (0.0 <= d) && (d <= 30.0))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&min_dist);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar sens = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &sens, sizeof(sens));
            if ((status == VX_SUCCESS) && (sens))
            {
                vx_enum type = 0;
                vxQueryScalar(sens, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    vx_float32 k = 0.0f;
                    vxCopyScalar(sens, &k, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    VX_PRINT(VX_ZONE_INFO, "k = %lf\n", k);
                    if ((0.040000f <= k) && (k < 0.150001f))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&sens);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 4 || index == 5)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar scalar = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if ((status == VX_SUCCESS) && (scalar))
            {
                vx_enum type = 0;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_INT32)
                {
                    vx_int32 size = 0;
                    vxCopyScalar(scalar, &size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    VX_PRINT(VX_ZONE_INFO, "size = %u\n", size);
                    if ((size == 3) || (size == 5) || (size == 7))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxHarrisOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    (void)node;

    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 6)
    {
        ptr->type = VX_TYPE_ARRAY;
        ptr->dim.array.item_type = VX_TYPE_KEYPOINT;
        ptr->dim.array.capacity = 0; /* no defined capacity requirement */
        status = VX_SUCCESS;
    }
    else if (index == 7)
    {
        ptr->dim.scalar.type = VX_TYPE_SIZE;
        status = VX_SUCCESS;
    }
    return status;
}

static vx_status VX_CALLBACK vxHarrisInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (num == dimof(harris_kernel_params))
    {
        vx_context context = vxGetContext((vx_reference)node);

        vx_graph subgraph = node->child;
        if (subgraph)
        {
            /* deallocate subgraph resources */
            status = vxReleaseGraph(&subgraph);
            if (VX_SUCCESS != status)
                return status;

            status = ownSetChildGraphOfNode(node, 0);
            if (VX_SUCCESS != status)
                return status;

            status = vxUnloadKernels(context, "openvx-extras");
            if (VX_SUCCESS != status)
                return status;

            status = vxUnloadKernels(context, "openvx-debug");
            if (VX_SUCCESS != status)
                return status;
        }

        status = vxLoadKernels(context, "openvx-extras");
        if (VX_SUCCESS != status)
            return status;

        status = vxLoadKernels(context, "openvx-debug");
        if (VX_SUCCESS != status)
            return status;

        /* allocate subgraph resources */
        subgraph = vxCreateGraph(context);

        status = vxGetStatus((vx_reference)subgraph);
        if (status == VX_SUCCESS)
        {
            vx_uint32 i = 0;

            vx_image  src = (vx_image)parameters[0];
            vx_scalar str = (vx_scalar)parameters[1];
            vx_scalar min = (vx_scalar)parameters[2];
            vx_scalar sen = (vx_scalar)parameters[3];
            vx_scalar win = (vx_scalar)parameters[4];
            vx_scalar blk = (vx_scalar)parameters[5];
            vx_array  arr = (vx_array)parameters[6];
            vx_scalar num_corners = (vx_scalar)parameters[7];

            vx_int32 shift_val = 4;
            vx_scalar shift = vxCreateScalar(context, VX_TYPE_INT32, &shift_val);

            vx_image virts[] =
            {
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT), // Gx
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT), // Gy
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT), // Score
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT), // Suppressed
/*
#if defined(OPENVX_DEBUGGING)
                vxCreateVirtualImage(g, 0, 0, VX_DF_IMAGE_U8), // Shifted Suppressed Log10
#endif
*/
            };

            vx_node nodes[] =
            {
                vxSobelMxNNode(subgraph, src, win, virts[0], virts[1]),
                vxHarrisScoreNode(subgraph, virts[0], virts[1], sen, win, blk, virts[2]),
                vxEuclideanNonMaxHarrisNode(subgraph, virts[2], str, min, virts[3]),
                vxImageListerNode(subgraph, virts[3], arr, num_corners),
/*
#if defined(OPENVX_DEBUGGING)
                vxConvertDepthNode(g,virts[3],virts[4],VX_CONVERT_POLICY_WRAP,shift),
                vxFWriteImageNode(g,virts[4],"oharris_strength_power_up4.pgm"),
#endif
*/
            };

            /* We have a child-graph; we want to make sure the parent
            graph is recognized as a valid scope for sake of virtual
            image parameters. */
            subgraph->parentGraph = node->graph;

            status = VX_SUCCESS;
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[0], 0); // src
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[2], 1); // str
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[2], 2); // min
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[1], 2); // sen
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[0], 1); // win
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[1], 3); // blk
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[3], 1); // arr
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[3], 2); // num_corners

            for (i = 0; i < dimof(nodes); i++)
            {
                status |= vxReleaseNode(&nodes[i]);
            }

            for (i = 0; i < dimof(virts); i++)
            {
                status |= vxReleaseImage(&virts[i]);
            }

            status |= vxReleaseScalar(&shift);

            status |= vxVerifyGraph(subgraph);
            VX_PRINT(VX_ZONE_INFO, "Status from Child Graph = %d\n", status);

            status |= ownSetChildGraphOfNode(node, subgraph);
        }
    }

    return status;
}

static vx_status VX_CALLBACK vxHarrisDeinitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    (void)parameters;

    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (num == dimof(harris_kernel_params))
    {
        vx_graph subgraph = ownGetChildGraphOfNode(node);
        vx_context context = vxGetContext((vx_reference)node);

        status = VX_SUCCESS;

        status |= vxReleaseGraph(&subgraph);

        /* set subgraph to "null" */
        status |= ownSetChildGraphOfNode(node, 0);

        status |= vxUnloadKernels(context, "openvx-extras");
        status |= vxUnloadKernels(context, "openvx-debug");
    }

    return status;
}

vx_kernel_description_t harris_kernel =
{
    VX_KERNEL_HARRIS_CORNERS,
    "org.khronos.openvx.harris_corners",
    vxHarrisCornersKernel,
    harris_kernel_params, dimof(harris_kernel_params),
    NULL,
    vxHarrisInputValidator,
    vxHarrisOutputValidator,
    vxHarrisInitializer,
    vxHarrisDeinitializer,
};
