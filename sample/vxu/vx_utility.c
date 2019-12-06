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
 * \brief The sample implementation of the immediate mode calls.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vxu.h>
#include <VX/vx_helper.h>
#include "vx_internal.h"

static vx_status setNodeTarget(vx_node node)
{
    vx_context context = node->base.context;
    return vxSetNodeTarget(node, context->imm_target_enum, context->imm_target_string);
}

VX_API_ENTRY vx_status VX_API_CALL vxuColorConvert(vx_context context, vx_image src, vx_image dst)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxColorConvertNode(graph, src, dst);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuChannelExtract(vx_context context, vx_image src, vx_enum channel, vx_image dst)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxChannelExtractNode(graph, src, channel, dst);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuChannelCombine(vx_context context,
                            vx_image plane0,
                            vx_image plane1,
                            vx_image plane2,
                            vx_image plane3,
                            vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxChannelCombineNode(graph, plane0, plane1, plane2, plane3, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

static const vx_enum border_modes_2[] =
{
    VX_BORDER_UNDEFINED,
    VX_BORDER_CONSTANT
};

static const vx_enum border_modes_3[] =
{
    VX_BORDER_UNDEFINED,
    VX_BORDER_CONSTANT,
    VX_BORDER_REPLICATE
};

static vx_status vx_isBorderModeSupported(vx_enum mode_to_check, const vx_enum supported_modes[], vx_size num_modes)
{
    vx_status status = VX_ERROR_NOT_SUPPORTED;
    vx_uint32 m = 0;
    for ( m = 0; m < num_modes; m++)
    {
        if (mode_to_check == supported_modes[m])
        {
            status = VX_SUCCESS;
            break;
        }
    }
    return status;
}

static vx_status vx_useImmediateBorderMode(vx_context context, vx_node node, const vx_enum supported_modes[], vx_size num_modes)
{
    vx_border_t border;
    vx_status status = vxQueryContext(context, VX_CONTEXT_IMMEDIATE_BORDER, &border, sizeof(border));
    if (status == VX_SUCCESS)
    {
        status = vx_isBorderModeSupported(border.mode, supported_modes, num_modes);
        if (status == VX_ERROR_NOT_SUPPORTED)
        {
            vx_enum policy;
            status = vxQueryContext(context, VX_CONTEXT_IMMEDIATE_BORDER_POLICY, &policy, sizeof(policy));
            if (status == VX_SUCCESS)
            {
                switch (policy)
                {
                    case VX_BORDER_POLICY_DEFAULT_TO_UNDEFINED:
                        border.mode = VX_BORDER_UNDEFINED;
                        status = VX_SUCCESS;
                        break;
                    case VX_BORDER_POLICY_RETURN_ERROR:
                        status = VX_ERROR_NOT_SUPPORTED;
                        break;
                    default:
                        status = VX_ERROR_NOT_SUPPORTED;
                        break;
                }
            }
        }
    }
    if (status == VX_SUCCESS)
        status = vxSetNodeAttribute(node, VX_NODE_BORDER, &border, sizeof(border));
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuSobel3x3(vx_context context, vx_image src, vx_image output_x, vx_image output_y)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxSobel3x3Node(graph, src, output_x, output_y);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMagnitude(vx_context context, vx_image grad_x, vx_image grad_y, vx_image dst)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxMagnitudeNode(graph, grad_x, grad_y, dst);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuPhase(vx_context context, vx_image grad_x, vx_image grad_y, vx_image dst)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxPhaseNode(graph, grad_x, grad_y, dst);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuScaleImage(vx_context context, vx_image src, vx_image dst, vx_enum type)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxScaleImageNode(graph, src, dst, type);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuTableLookup(vx_context context, vx_image input, vx_lut lut, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTableLookupNode(graph, input, lut, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuHistogram(vx_context context, vx_image input, vx_distribution distribution)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxHistogramNode(graph, input, distribution);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuEqualizeHist(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxEqualizeHistNode(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuAbsDiff(vx_context context, vx_image in1, vx_image in2, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxAbsDiffNode(graph, in1, in2, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMeanStdDev(vx_context context, vx_image input, vx_float32 *mean, vx_float32 *stddev)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_scalar s_mean = vxCreateScalar(context, VX_TYPE_FLOAT32, NULL);
        vx_scalar s_stddev = vxCreateScalar(context, VX_TYPE_FLOAT32, NULL);
        vx_node node = vxMeanStdDevNode(graph, input, s_mean, s_stddev);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
                vxCopyScalar(s_mean, mean, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxCopyScalar(s_stddev, stddev, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            }
            vxReleaseNode(&node);
        }
        vxReleaseScalar(&s_mean);
        vxReleaseScalar(&s_stddev);
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuThreshold(vx_context context, vx_image input, vx_threshold thresh, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxThresholdNode(graph, input, thresh, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuIntegralImage(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxIntegralImageNode(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuErode3x3(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxErode3x3Node(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuDilate3x3(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxDilate3x3Node(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMedian3x3(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxMedian3x3Node(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuBox3x3(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxBox3x3Node(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuGaussian3x3(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxGaussian3x3Node(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuNonLinearFilter(vx_context context, vx_enum function, vx_image input, vx_matrix mask, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxNonLinearFilterNode(graph, function, input, mask, output);
        if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuConvolve(vx_context context, vx_image input, vx_convolution conv, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxConvolveNode(graph, input, conv, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuGaussianPyramid(vx_context context, vx_image input, vx_pyramid gaussian)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxGaussianPyramidNode(graph, input, gaussian);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuLaplacianPyramid(vx_context context, vx_image input, vx_pyramid laplacian, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxLaplacianPyramidNode(graph, input, laplacian, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuLaplacianReconstruct(vx_context context, vx_pyramid laplacian, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxLaplacianReconstructNode(graph, laplacian, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_2, dimof(border_modes_2));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuAccumulateImage(vx_context context, vx_image input, vx_image accum)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxAccumulateImageNode(graph, input, accum);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuAccumulateWeightedImage(vx_context context, vx_image input, vx_scalar scale, vx_image accum)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxAccumulateWeightedImageNode(graph, input, scale, accum);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuAccumulateSquareImage(vx_context context, vx_image input, vx_scalar scale, vx_image accum)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxAccumulateSquareImageNode(graph, input, scale, accum);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMinMaxLoc(vx_context context, vx_image input,
                        vx_scalar minVal, vx_scalar maxVal,
                        vx_array minLoc, vx_array maxLoc,
                        vx_scalar minCount, vx_scalar maxCount)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxMinMaxLocNode(graph, input, minVal, maxVal, minLoc, maxLoc, minCount, maxCount);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuWeightedAverage(vx_context context, vx_image img1, vx_scalar alpha, vx_image img2, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxWeightedAverageNode(graph, img1, alpha, img2, output);
        if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuConvertDepth(vx_context context, vx_image input, vx_image output, vx_enum policy, vx_int32 shift_val)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);

    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_scalar shift = vxCreateScalar(context, VX_TYPE_INT32, &shift_val);
        vx_node node = vxConvertDepthNode(graph, input, output, policy, shift);
        if ( (VX_SUCCESS == vxGetStatus((vx_reference)node)) &&
             (VX_SUCCESS == vxGetStatus((vx_reference)shift)) )
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
        vxReleaseScalar(&shift);
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuCannyEdgeDetector(vx_context context, vx_image input, vx_threshold hyst,
                               vx_int32 gradient_size, vx_enum norm_type,
                               vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxCannyEdgeDetectorNode(graph, input, hyst, gradient_size, norm_type, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuHalfScaleGaussian(vx_context context, vx_image input, vx_image output, vx_int32 kernel_size)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxHalfScaleGaussianNode(graph, input, output, kernel_size);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_2, dimof(border_modes_2));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuAnd(vx_context context, vx_image in1, vx_image in2, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxAndNode(graph, in1, in2, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuOr(vx_context context, vx_image in1, vx_image in2, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxOrNode(graph, in1, in2, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuXor(vx_context context, vx_image in1, vx_image in2, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxXorNode(graph, in1, in2, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuNot(vx_context context, vx_image input, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxNotNode(graph, input, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMultiply(vx_context context, vx_image in1, vx_image in2, vx_float32 scale_val, vx_enum overflow_policy, vx_enum rounding_policy, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);

    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_scalar scale = vxCreateScalar(context, VX_TYPE_FLOAT32, &scale_val);
        vx_node node = vxMultiplyNode(graph, in1, in2, scale, overflow_policy, rounding_policy, out);
        if ( (VX_SUCCESS == vxGetStatus((vx_reference)node)) &&
             (VX_SUCCESS == vxGetStatus((vx_reference)scale)) )
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
        vxReleaseScalar(&scale);
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuAdd(vx_context context, vx_image in1, vx_image in2, vx_enum policy, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxAddNode(graph, in1, in2, policy, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuSubtract(vx_context context, vx_image in1, vx_image in2, vx_enum policy, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxSubtractNode(graph, in1, in2, policy, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuTensorMultiply(vx_context context, vx_tensor input1, vx_tensor input2, vx_scalar scale, vx_enum overflow_policy,
        vx_enum rounding_policy, vx_tensor output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTensorMultiplyNode(graph, input1, input2, scale, overflow_policy, rounding_policy, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;

}


VX_API_ENTRY vx_status VX_API_CALL vxuTensorAdd(vx_context context, vx_tensor input1, vx_tensor input2, vx_enum policy, vx_tensor output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTensorAddNode(graph, input1, input2, policy, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;

}

VX_API_ENTRY vx_status VX_API_CALL vxuTensorSubtract(vx_context context, vx_tensor input1, vx_tensor input2, vx_enum policy, vx_tensor output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTensorSubtractNode(graph, input1, input2, policy, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;


}

VX_API_ENTRY vx_status VX_API_CALL vxuTensorTableLookup(vx_context context, vx_tensor input1, vx_lut lut, vx_tensor output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTensorTableLookupNode(graph, input1, lut, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;

}

VX_API_ENTRY vx_status VX_API_CALL vxuTensorTranspose(vx_context context, vx_tensor input, vx_tensor output, vx_size dimension1, vx_size dimension2)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTensorTransposeNode(graph, input, output, dimension1, dimension2);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;

}

VX_API_ENTRY vx_status VX_API_CALL vxuTensorConvertDepth(vx_context context, vx_tensor input, vx_enum policy, vx_scalar norm, vx_scalar offset, vx_tensor output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTensorConvertDepthNode(graph, input, policy, norm, offset, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;

}

VX_API_ENTRY vx_status VX_API_CALL vxuTensorMatrixMultiply(vx_context context, vx_tensor input1, vx_tensor input2, vx_tensor input3,
    const vx_tensor_matrix_multiply_params_t *matrix_multiply_params, vx_tensor output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxTensorMatrixMultiplyNode(graph, input1, input2, input3, matrix_multiply_params, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;


}

VX_API_ENTRY vx_status VX_API_CALL vxuWarpAffine(vx_context context, vx_image input, vx_matrix matrix, vx_enum type, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxWarpAffineNode(graph, input, matrix, type, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_2, dimof(border_modes_2));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuWarpPerspective(vx_context context, vx_image input, vx_matrix matrix, vx_enum type, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxWarpPerspectiveNode(graph, input, matrix, type, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_2, dimof(border_modes_2));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuHarrisCorners(vx_context context, vx_image input,
        vx_scalar strength_thresh,
        vx_scalar min_distance,
        vx_scalar sensitivity,
        vx_int32 gradient_size,
        vx_int32 block_size,
        vx_array corners,
        vx_scalar num_corners)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxHarrisCornersNode(graph, input, strength_thresh, min_distance, sensitivity, gradient_size, block_size, corners, num_corners);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuFastCorners(vx_context context, vx_image input, vx_scalar sens, vx_bool nonmax, vx_array corners, vx_scalar num_corners)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxFastCornersNode(graph, input, sens, nonmax, corners, num_corners);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuNonMaxSuppression(vx_context context, vx_image input, vx_image mask, vx_int32 win_size, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxNonMaxSuppressionNode(graph, input, mask, win_size, output);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuOpticalFlowPyrLK(vx_context context, vx_pyramid old_images,
                              vx_pyramid new_images,
                              vx_array old_points,
                              vx_array new_points_estimates,
                              vx_array new_points,
                              vx_enum termination,
                              vx_scalar epsilon,
                              vx_scalar num_iterations,
                              vx_scalar use_initial_estimate,
                              vx_size window_dimension)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxOpticalFlowPyrLKNode(graph, old_images, new_images, old_points,new_points_estimates, new_points,
                termination,epsilon,num_iterations,use_initial_estimate,window_dimension);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuRemap(vx_context context, vx_image input, vx_remap table, vx_enum policy, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxRemapNode(graph, input, table, policy, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_2, dimof(border_modes_2));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMin(vx_context context, vx_image in1, vx_image in2, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxMinNode(graph, in1, in2, out);
	    if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMax(vx_context context, vx_image in1, vx_image in2, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxMaxNode(graph, in1, in2, out);
	    if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
            {
               status = vxVerifyGraph(graph);
                if (status == VX_SUCCESS)
                {
                    status = vxProcessGraph(graph);
                }
                vxReleaseNode(&node);
            }
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuLBP(vx_context context,
    vx_image in, vx_enum format, vx_int8 kernel_size, vx_image out)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxLBPNode(graph, in, format, kernel_size, out);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuBilateralFilter(vx_context context, vx_tensor src, vx_int32 diameter,
                              vx_float32 sigmaSpace,
                              vx_float32 sigmaValues,
                              vx_tensor dst)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxBilateralFilterNode(graph, src, diameter, sigmaSpace, sigmaValues, dst);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vx_useImmediateBorderMode(context, node, border_modes_3, dimof(border_modes_3));
            if (status == VX_SUCCESS)
                status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuMatchTemplate(vx_context context,
                                                        vx_image src,
                                                        vx_image templateImage,
                                                        vx_enum matchingMethod,
                                                        vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxMatchTemplateNode(graph, src, templateImage, matchingMethod, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = setNodeTarget(node);
            if (status == VX_SUCCESS)
                status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuHoughLinesP(vx_context context,
    vx_image input,
    const vx_hough_lines_p_t *params,
    vx_array lines_array,
    vx_scalar num_lines)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (graph)
    {
        vx_node node = vxHoughLinesPNode(graph, input, params, lines_array, num_lines);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxuCopy(vx_context context, vx_reference input, vx_reference output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxCopyNode(graph, input, output);
        if (vxGetStatus((vx_reference)node)==VX_SUCCESS)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_status VX_API_CALL vxuHOGCells(vx_context context, vx_image input, vx_int32 cell_width, vx_int32 cell_height, vx_int32 num_bins, vx_tensor magnitudes, vx_tensor bins)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxHOGCellsNode(graph, input, cell_width, cell_height, num_bins, magnitudes, bins);
        if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_status VX_API_CALL vxuHOGFeatures(vx_context context, vx_image input, vx_tensor magnitudes, vx_tensor bins, const vx_hog_t *params, vx_size hog_param_size, vx_tensor features)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxHOGFeaturesNode(graph, input, magnitudes, bins, params, hog_param_size, features);
        if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}
