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
 * \brief The Canny Edge Detector Kernel Implementation.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <math.h>

/*! \note Look at \ref vxCannyEdgeDetectorNode to see how this pyramid construction works */

static vx_param_description_t canny_kernel_params[] =
{
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_THRESHOLD, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};


static vx_status VX_CALLBACK vxCannyEdgeKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    (void)parameters;

    vx_status status = VX_FAILURE;

    if (num == dimof(canny_kernel_params))
    {
        vx_graph subgraph = ownGetChildGraphOfNode(node);
        status = vxProcessGraph(subgraph);
    }

    return status;
}


static vx_status VX_CALLBACK vxCannyEdgeInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_threshold hyst = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &hyst, sizeof(hyst));
        if (hyst)
        {
            vx_enum type = 0;
            vxQueryThreshold(hyst, VX_THRESHOLD_TYPE, &type, sizeof(type));
            if (type == VX_THRESHOLD_TYPE_RANGE)
            {
                vx_enum data_type = 0;
                vxQueryThreshold(hyst, VX_THRESHOLD_DATA_TYPE, &data_type, sizeof(data_type));
                if (data_type == VX_TYPE_UINT8 || data_type == VX_TYPE_INT16)
                {
                    status = VX_SUCCESS;
                }
            }
            vxReleaseThreshold(&hyst);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar value = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &value, sizeof(value));
            if (value)
            {
                vx_enum stype = 0;
                vxQueryScalar(value, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_INT32)
                {
                    vx_int32 gs = 0;
                    vxCopyScalar(value, &gs, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((gs == 3) || (gs == 5) || (gs == 7))
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
                vxReleaseScalar(&value);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar value = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &value, sizeof(value));
            if (value)
            {
                vx_enum norm = 0;
                vxCopyScalar(value, &norm, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                if ((norm == VX_NORM_L1) ||
                    (norm == VX_NORM_L2))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseScalar(&value);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}


static vx_status VX_CALLBACK vxCannyEdgeOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 4)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        if (vxGetStatus((vx_reference)src_param) == VX_SUCCESS)
        {
            vx_image src = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            if (src)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = VX_DF_IMAGE_VIRT;

                vxQueryImage(src, VX_IMAGE_WIDTH, &width, sizeof(height));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &height, sizeof(height));
                vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = format;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&src);
            }
            vxReleaseParameter(&src_param);
        }
    }
    return status;
}


static vx_status VX_CALLBACK vxCannyEdgeInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (num == dimof(canny_kernel_params))
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
            if (status != VX_SUCCESS)
                return status;
        }

        status = vxLoadKernels(context, "openvx-extras");
        if (status != VX_SUCCESS)
            return status;

        /* allocate subgraph resources */
        subgraph = vxCreateGraph(context);

        status = vxGetStatus((vx_reference)subgraph);
        if (status == VX_SUCCESS)
        {
            vx_uint32 i;

            vx_image     input = (vx_image)parameters[0];
            vx_threshold hyst = (vx_threshold)parameters[1];
            vx_scalar    gradient_size = (vx_scalar)parameters[2];
            vx_scalar    norm_type = (vx_scalar)parameters[3];
            vx_image     output = (vx_image)parameters[4];

            vx_image virts[] =
            {
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT),  /* 0: Gx */
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT),  /* 1: Gy */
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT),  /* 2: Mag */
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT),  /* 3: Phase */
                vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_VIRT),  /* 4: Nonmax */
            };

            vx_node nodes[] =
            {
                vxSobelMxNNode(subgraph, input, gradient_size, virts[0], virts[1]),
                vxElementwiseNormNode(subgraph, virts[0], virts[1], norm_type, virts[2]),
                vxPhaseNode(subgraph, virts[0], virts[1], virts[3]),
                vxNonMaxSuppressionCannyNode(subgraph, virts[2], virts[3], virts[4]),
                vxEdgeTraceNode(subgraph, virts[4], hyst, output),
            };

            vx_border_t borders;

            /* We have a child-graph; we want to make sure the parent
            graph is recognized as a valid scope for sake of virtual
            image parameters. */
            subgraph->parentGraph = node->graph;

            status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
            for (i = 0; i < dimof(nodes); i++)
            {
                status |= vxSetNodeAttribute(nodes[i], VX_NODE_BORDER, &borders, sizeof(borders));
            }

            status |= vxAddParameterToGraphByIndex(subgraph, nodes[0], 0); /* input image */
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[4], 1); /* threshold */
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[0], 1); /* gradient size */
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[1], 2); /* norm type */
            status |= vxAddParameterToGraphByIndex(subgraph, nodes[4], 2); /* output image */

            status |= vxVerifyGraph(subgraph);

            /* release our references, the graph will hold it's own */
            for (i = 0; i < dimof(virts); i++)
            {
                status |= vxReleaseImage(&virts[i]);
            }

            for (i = 0; i < dimof(nodes); i++)
            {
                status |= vxReleaseNode(&nodes[i]);
            }

            status |= ownSetChildGraphOfNode(node, subgraph);
        }
    }

    return status;
}


static vx_status VX_CALLBACK vxCannyEdgeDeinitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (num == dimof(canny_kernel_params))
    {
        vx_graph subgraph = ownGetChildGraphOfNode(node);
        vx_context context = vxGetContext((vx_reference)node);

        status = VX_SUCCESS;

        status |= vxReleaseGraph(&subgraph);

        /* set subgraph to "null" */
        status |= ownSetChildGraphOfNode(node, 0);

        status |= vxUnloadKernels(context, "openvx-extras");
    }

    return status;
}


vx_kernel_description_t canny_kernel =
{
    VX_KERNEL_CANNY_EDGE_DETECTOR,
    "org.khronos.openvx.canny_edge_detector",
    vxCannyEdgeKernel,
    canny_kernel_params, dimof(canny_kernel_params),
    NULL,
    vxCannyEdgeInputValidator,
    vxCannyEdgeOutputValidator,
    vxCannyEdgeInitializer,
    vxCannyEdgeDeinitializer,
};
