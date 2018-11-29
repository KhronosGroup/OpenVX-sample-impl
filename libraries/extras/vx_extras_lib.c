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
 * \brief The Khronos Extras Extension Node and Immediate Interfaces
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>

//*****************************************************************************
// PUBLIC INTERFACE
//*****************************************************************************

vx_node vxNonMaxSuppressionCannyNode(vx_graph graph, vx_image mag, vx_image phase, vx_image edge)
{
    vx_reference params[] = {
        (vx_reference)mag,
        (vx_reference)phase,
        (vx_reference)edge,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_EXTRAS_NONMAXSUPPRESSION_CANNY,
                                   params, dimof(params));
}

vx_node vxLaplacian3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_EXTRAS_LAPLACIAN_3x3,
                                   params,
                                   dimof(params));
}

vx_status vxuLaplacian3x3(vx_context context, vx_image input, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxLaplacian3x3Node(graph, input, output);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxClearLog((vx_reference)graph);
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_node vxScharr3x3Node(vx_graph graph, vx_image input, vx_image output1, vx_image output2)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output1,
        (vx_reference)output2
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_EXTRAS_SCHARR_3x3,
                                   params,
                                   dimof(params));
}

vx_status vxuScharr3x3(vx_context context, vx_image input, vx_image output1,vx_image output2)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxScharr3x3Node(graph, input, output1,output2);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxClearLog((vx_reference)graph);
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_node vxSobelMxNNode(vx_graph graph, vx_image input, vx_scalar ws, vx_image gx, vx_image gy)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)ws,
        (vx_reference)gx,
        (vx_reference)gy,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                   VX_KERNEL_EXTRAS_SOBEL_MxN,
                                   params,
                                   dimof(params));
    return node;
}

vx_status vxuSobelMxN(vx_context context, vx_image input, vx_scalar win, vx_image gx, vx_image gy)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxSobelMxNNode(graph, input, win, gx, gy);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxClearLog((vx_reference)graph);
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_node vxHarrisScoreNode(vx_graph	graph,
                          vx_image	gx,
                          vx_image	gy,
                          vx_scalar sensitivity,
                          vx_scalar grad_size,
                          vx_scalar block_size,
                          vx_image	score)
{
    vx_reference params[] = {
        (vx_reference)gx,
        (vx_reference)gy,
        (vx_reference)sensitivity,
        (vx_reference)grad_size,
        (vx_reference)block_size,
        (vx_reference)score,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_EXTRAS_HARRIS_SCORE,
                                           params,
                                           dimof(params));
    return node;
}

vx_status vxuHarrisScore(vx_context context,
                         vx_image	gx,
                         vx_image	gy,
                         vx_scalar	sensitivity,
                         vx_scalar	grad_size,
                         vx_scalar	block_size,
                         vx_image	score)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxHarrisScoreNode(graph, gx, gy, sensitivity, grad_size, block_size, score);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxClearLog((vx_reference)graph);
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_node vxEuclideanNonMaxHarrisNode(vx_graph graph,
                              vx_image input,
                              vx_scalar strength_thresh,
                              vx_scalar min_distance,
                              vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)strength_thresh,
        (vx_reference)min_distance,
        (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_EXTRAS_EUCLIDEAN_NONMAXSUPPRESSION_HARRIS,
                                           params,
                                           dimof(params));
    return node;
}

vx_status vxuEuclideanNonMaxHarris(vx_context context, vx_image input,
                             vx_scalar strength_thresh,
                             vx_scalar min_distance,
                             vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxEuclideanNonMaxHarrisNode(graph, input, strength_thresh, min_distance, output);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxClearLog((vx_reference)graph);
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_node vxImageListerNode(vx_graph graph, vx_image input, vx_array arr, vx_scalar num_points)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)arr,
        (vx_reference)num_points,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_EXTRAS_IMAGE_LISTER,
                                   params,
                                   dimof(params));
}

vx_status vxuImageLister(vx_context context, vx_image input,
                         vx_array arr, vx_scalar num_points)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxImageListerNode(graph, input, arr, num_points);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxClearLog((vx_reference)graph);
        vxReleaseGraph(&graph);
    }
    return status;
}

vx_node vxElementwiseNormNode(vx_graph graph,
                              vx_image input_x,
                              vx_image input_y,
                              vx_scalar norm_type,
                              vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input_x,
        (vx_reference)input_y,
        (vx_reference)norm_type,
        (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_EXTRAS_ELEMENTWISE_NORM,
                                           params,
                                           dimof(params));
    return node;
}

vx_node vxEdgeTraceNode(vx_graph graph,
                        vx_image norm,
                        vx_threshold threshold,
                        vx_image output)
{
    vx_reference params[] = {
        (vx_reference)norm,
        (vx_reference)threshold,
        (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_EXTRAS_EDGE_TRACE,
                                           params,
                                           dimof(params));
    return node;
}
