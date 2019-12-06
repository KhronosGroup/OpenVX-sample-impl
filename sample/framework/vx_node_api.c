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
 * \brief The Graph Mode Interface for all Base Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include "vx_internal.h"

VX_API_ENTRY vx_node VX_API_CALL vxColorConvertNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph, VX_KERNEL_COLOR_CONVERT, params, dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxChannelExtractNode(vx_graph graph,
                             vx_image input,
                             vx_enum channelNum,
                             vx_image output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar scalar = vxCreateScalar(context, VX_TYPE_ENUM, &channelNum);
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)scalar,
        (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_CHANNEL_EXTRACT,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&scalar); // node hold reference
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxChannelCombineNode(vx_graph graph,
                             vx_image plane0,
                             vx_image plane1,
                             vx_image plane2,
                             vx_image plane3,
                             vx_image output)
{
    vx_reference params[] = {
       (vx_reference)plane0,
       (vx_reference)plane1,
       (vx_reference)plane2,
       (vx_reference)plane3,
       (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_CHANNEL_COMBINE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxSobel3x3Node(vx_graph graph, vx_image input, vx_image output_x, vx_image output_y)
{
    vx_reference params[] = {
       (vx_reference)input,
       (vx_reference)output_x,
       (vx_reference)output_y,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_SOBEL_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMagnitudeNode(vx_graph graph, vx_image grad_x, vx_image grad_y, vx_image mag)
{
    vx_reference params[] = {
       (vx_reference)grad_x,
       (vx_reference)grad_y,
       (vx_reference)mag,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_MAGNITUDE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxPhaseNode(vx_graph graph, vx_image grad_x, vx_image grad_y, vx_image orientation)
{
    vx_reference params[] = {
       (vx_reference)grad_x,
       (vx_reference)grad_y,
       (vx_reference)orientation,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_PHASE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxScaleImageNode(vx_graph graph, vx_image src, vx_image dst, vx_enum type)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar stype = vxCreateScalar(context, VX_TYPE_ENUM, &type);
    vx_reference params[] = {
        (vx_reference)src,
        (vx_reference)dst,
        (vx_reference)stype,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_SCALE_IMAGE,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&stype);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxTableLookupNode(vx_graph graph, vx_image input, vx_lut lut, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)lut,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_TABLE_LOOKUP,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxHistogramNode(vx_graph graph, vx_image input, vx_distribution distribution)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)distribution,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_HISTOGRAM,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxEqualizeHistNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_EQUALIZE_HISTOGRAM,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAbsDiffNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_ABSDIFF,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMeanStdDevNode(vx_graph graph, vx_image input, vx_scalar mean, vx_scalar stddev)
{
    vx_reference params[] = {
       (vx_reference)input,
       (vx_reference)mean,
       (vx_reference)stddev,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_MEAN_STDDEV,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxThresholdNode(vx_graph graph, vx_image input, vx_threshold thesh, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)thesh,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_THRESHOLD,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxIntegralImageNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_INTEGRAL_IMAGE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxErode3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_ERODE_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxDilate3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_DILATE_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMedian3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_MEDIAN_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxBox3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_BOX_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxGaussian3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_GAUSSIAN_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxNonLinearFilterNode(vx_graph graph, vx_enum function, vx_image input, vx_matrix mask, vx_image output)
{
    vx_scalar func = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_ENUM, &function);

    vx_reference params[] = {
        (vx_reference)func,
        (vx_reference)input,
        (vx_reference)mask,
        (vx_reference)output,
    };

    vx_node node = vxCreateNodeByStructure(graph,
        VX_KERNEL_NON_LINEAR_FILTER,
        params,
        dimof(params));

    vxReleaseScalar(&func);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxConvolveNode(vx_graph graph, vx_image input, vx_convolution conv, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)conv,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_CUSTOM_CONVOLUTION,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxGaussianPyramidNode(vx_graph graph, vx_image input, vx_pyramid gaussian)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)gaussian,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_GAUSSIAN_PYRAMID,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxLaplacianPyramidNode(vx_graph graph, vx_image input, vx_pyramid laplacian, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)laplacian,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_LAPLACIAN_PYRAMID,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxLaplacianReconstructNode(vx_graph graph, vx_pyramid laplacian, vx_image input,
                                       vx_image output)
{
    vx_reference params[] = {
        (vx_reference)laplacian,
        (vx_reference)input,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_LAPLACIAN_RECONSTRUCT,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateImageNode(vx_graph graph, vx_image input, vx_image accum)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)accum,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_ACCUMULATE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateWeightedImageNode(vx_graph graph, vx_image input, vx_scalar alpha, vx_image accum)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)alpha,
        (vx_reference)accum,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_ACCUMULATE_WEIGHTED,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateSquareImageNode(vx_graph graph, vx_image input, vx_scalar scalar, vx_image accum)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)scalar,
        (vx_reference)accum,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_ACCUMULATE_SQUARE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMinMaxLocNode(vx_graph graph,
                        vx_image input,
                        vx_scalar minVal, vx_scalar maxVal,
                        vx_array minLoc, vx_array maxLoc,
                        vx_scalar minCount, vx_scalar maxCount)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)minVal,
        (vx_reference)maxVal,
        (vx_reference)minLoc,
        (vx_reference)maxLoc,
        (vx_reference)minCount,
        (vx_reference)maxCount,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_MINMAXLOC,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxWeightedAverageNode(vx_graph graph, vx_image img1, vx_scalar alpha, vx_image img2, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)img1,
        (vx_reference)alpha,
        (vx_reference)img2,
        (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
        VX_KERNEL_WEIGHTED_AVERAGE,
        params,
        dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxConvertDepthNode(vx_graph graph, vx_image input, vx_image output, vx_enum policy, vx_scalar shift)
{
    vx_scalar pol = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
        (vx_reference)pol,
        (vx_reference)shift,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                   VX_KERNEL_CONVERTDEPTH,
                                   params,
                                   dimof(params));
    vxReleaseScalar(&pol);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxCannyEdgeDetectorNode(vx_graph graph, vx_image input, vx_threshold hyst,
                                vx_int32 gradient_size, vx_enum norm_type,
                                vx_image output)
{
    vx_scalar gs = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_INT32, &gradient_size);
    vx_scalar nt = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_ENUM, &norm_type);
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)hyst,
        (vx_reference)gs,
        (vx_reference)nt,
        (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_CANNY_EDGE_DETECTOR,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&gs);
    vxReleaseScalar(&nt);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxAndNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_AND,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxOrNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_OR,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxXorNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_XOR,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxNotNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
       (vx_reference)input,
       (vx_reference)output,
    };
    return vxCreateNodeByStructure(graph,
                                   VX_KERNEL_NOT,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMultiplyNode(vx_graph graph, vx_image in1, vx_image in2, vx_scalar scale, vx_enum overflow_policy, vx_enum rounding_policy, vx_image out)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, VX_TYPE_ENUM, &overflow_policy);
    vx_scalar rpolicy = vxCreateScalar(context, VX_TYPE_ENUM, &rounding_policy);
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)scale,
       (vx_reference)spolicy,
       (vx_reference)rpolicy,
       (vx_reference)out,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_MULTIPLY,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&spolicy);
    vxReleaseScalar(&rpolicy);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxAddNode(vx_graph graph, vx_image in1, vx_image in2, vx_enum policy, vx_image out)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)spolicy,
       (vx_reference)out,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_ADD,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&spolicy);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxSubtractNode(vx_graph graph, vx_image in1, vx_image in2, vx_enum policy, vx_image out)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)spolicy,
       (vx_reference)out,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_SUBTRACT,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&spolicy);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxWarpAffineNode(vx_graph graph, vx_image input, vx_matrix matrix, vx_enum type, vx_image output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar stype = vxCreateScalar(context, VX_TYPE_ENUM, &type);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)matrix,
            (vx_reference)stype,
            (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_WARP_AFFINE,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&stype);

    if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
    {
        /* default value for Warp node */
        /* change node attribute as kernel attributes alreay copied to node */
        /* in vxCreateNodeByStructure() */
        node->attributes.valid_rect_reset = vx_true_e;
    }

    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxWarpPerspectiveNode(vx_graph graph, vx_image input, vx_matrix matrix, vx_enum type, vx_image output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar stype = vxCreateScalar(context, VX_TYPE_ENUM, &type);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)matrix,
            (vx_reference)stype,
            (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_WARP_PERSPECTIVE,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&stype);

    if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
    {
        /* default value for Warp node */
        /* change node attribute as kernel attributes alreay copied to node */
        /* in vxCreateNodeByStructure() */
        node->attributes.valid_rect_reset = vx_true_e;
    }

    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxHarrisCornersNode(vx_graph graph,
                            vx_image input,
                            vx_scalar strength_thresh,
                            vx_scalar min_distance,
                            vx_scalar sensitivity,
                            vx_int32 gradient_size,
                            vx_int32 block_size,
                            vx_array corners,
                            vx_scalar num_corners)
{
    vx_scalar win = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_INT32, &gradient_size);
    vx_scalar blk = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_INT32, &block_size);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)strength_thresh,
            (vx_reference)min_distance,
            (vx_reference)sensitivity,
            (vx_reference)win,
            (vx_reference)blk,
            (vx_reference)corners,
            (vx_reference)num_corners,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_HARRIS_CORNERS,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&win);
    vxReleaseScalar(&blk);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxFastCornersNode(vx_graph graph, vx_image input, vx_scalar strength_thresh, vx_bool nonmax_suppression, vx_array corners, vx_scalar num_corners)
{
    vx_scalar nonmax = vxCreateScalar(vxGetContext((vx_reference)graph),VX_TYPE_BOOL, &nonmax_suppression);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)strength_thresh,
            (vx_reference)nonmax,
            (vx_reference)corners,
            (vx_reference)num_corners,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_FAST_CORNERS,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&nonmax);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxNonMaxSuppressionNode(vx_graph graph, vx_image input, vx_image mask, vx_int32 win_size, vx_image output)
{
    vx_scalar wsize = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_INT32, &win_size);
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)mask,
        (vx_reference)wsize,
        (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
        VX_KERNEL_NON_MAX_SUPPRESSION,
        params,
        dimof(params));
    vxReleaseScalar(&wsize);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxOpticalFlowPyrLKNode(vx_graph graph,
                               vx_pyramid old_images,
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
    vx_scalar term = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_ENUM, &termination);
    vx_scalar winsize = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_SIZE, &window_dimension);
    vx_reference params[] = {
            (vx_reference)old_images,
            (vx_reference)new_images,
            (vx_reference)old_points,
            (vx_reference)new_points_estimates,
            (vx_reference)new_points,
            (vx_reference)term,
            (vx_reference)epsilon,
            (vx_reference)num_iterations,
            (vx_reference)use_initial_estimate,
            (vx_reference)winsize,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_OPTICAL_FLOW_PYR_LK,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&term);
    vxReleaseScalar(&winsize);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxRemapNode(vx_graph graph,
                    vx_image input,
                    vx_remap table,
                    vx_enum policy,
                    vx_image output)
{
    vx_scalar spolicy = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)table,
            (vx_reference)spolicy,
            (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_REMAP,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&spolicy);

    if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
    {
        /* default value for Remap node */
        /* change node attribute as kernel attributes alreay copied to node */
        /* in vxCreateNodeByStructure() */
        node->attributes.valid_rect_reset = vx_true_e;
    }

    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxHalfScaleGaussianNode(vx_graph graph, vx_image input, vx_image output, vx_int32 kernel_size)
{
    vx_scalar ksize = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_INT32, &kernel_size);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)output,
            (vx_reference)ksize,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_HALFSCALE_GAUSSIAN,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&ksize);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxHoughLinesPNode(vx_graph graph, vx_image input, const vx_hough_lines_p_t *params_hough_lines, vx_array lines_array, vx_scalar num_lines)
{
    vx_array params_hough_lines_array = vxCreateArray(vxGetContext((vx_reference)graph), VX_TYPE_HOUGH_LINES_PARAMS, 1);
    vxAddArrayItems(params_hough_lines_array, 1, params_hough_lines, sizeof(vx_hough_lines_p_t));
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)params_hough_lines_array,
        (vx_reference)lines_array,
        (vx_reference)num_lines,
    };
    vx_node node = vxCreateNodeByStructure(graph,
        VX_KERNEL_HOUGH_LINES_P,
        params,
        dimof(params));
    vxReleaseArray(&params_hough_lines_array);
    return node;
}

/*==============================================================================
vxTensorMultiplyNode
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxTensorMultiplyNode(vx_graph graph, vx_tensor input1, vx_tensor input2, vx_scalar scale, vx_enum overflow_policy,
        vx_enum rounding_policy, vx_tensor output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, VX_TYPE_ENUM, &overflow_policy);
    vx_scalar rpolicy = vxCreateScalar(context, VX_TYPE_ENUM, &rounding_policy);
    vx_reference params[] = {
        (vx_reference)input1,
        (vx_reference)input2,
        (vx_reference)scale,
        (vx_reference)spolicy,
        (vx_reference)rpolicy,
        (vx_reference)output,
    };

    vx_node node = NULL;

    node = vxCreateNodeByStructure(graph, VX_KERNEL_TENSOR_MULTIPLY, params, dimof(params));

    vxReleaseScalar(&spolicy);
    vxReleaseScalar(&rpolicy);
    return node;
}

/*==============================================================================
vxTensorAddNode
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxTensorAddNode(vx_graph graph, vx_tensor input1, vx_tensor input2, vx_enum policy, vx_tensor output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
        (vx_reference)input1,
        (vx_reference)input2,
        (vx_reference)spolicy,
        (vx_reference)output,
    };

    vx_node node = NULL;

    node = vxCreateNodeByStructure(graph, VX_KERNEL_TENSOR_ADD, params, dimof(params));

    vxReleaseScalar(&spolicy);
    return node;
}

/*==============================================================================
vxTensorAddNode
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxTensorSubtractNode(vx_graph graph, vx_tensor input1, vx_tensor input2, vx_enum policy, vx_tensor output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
        (vx_reference)input1,
        (vx_reference)input2,
        (vx_reference)spolicy,
        (vx_reference)output,
    };

    vx_node node = NULL;

    node = vxCreateNodeByStructure(graph, VX_KERNEL_TENSOR_SUBTRACT, params, dimof(params));

    vxReleaseScalar(&spolicy);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxTensorTableLookupNode(vx_graph graph, vx_tensor input, vx_lut lut, vx_tensor output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)lut,
        (vx_reference)output,
    };

    return vxCreateNodeByStructure(graph, VX_KERNEL_TENSOR_TABLE_LOOKUP, params, dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxTensorTransposeNode(vx_graph graph, vx_tensor input, vx_tensor output, vx_size dimension0, vx_size dimension1)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar dim1 = vxCreateScalar(context, VX_TYPE_SIZE, &dimension0);
    vx_scalar dim2 = vxCreateScalar(context, VX_TYPE_SIZE, &dimension1);

    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)dim1,
        (vx_reference)dim2,
        (vx_reference)output,
    };

    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_TENSOR_TRANSPOSE, params, dimof(params));

    vxReleaseScalar(&dim1);
    vxReleaseScalar(&dim2);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxTensorConvertDepthNode(vx_graph graph, vx_tensor input, vx_enum policy, vx_scalar norm, vx_scalar offset, vx_tensor output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar overflow_policy_sc = vxCreateScalar(context, VX_TYPE_ENUM, &policy);

    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)overflow_policy_sc,
        (vx_reference)norm,
        (vx_reference)offset,
        (vx_reference)output,
    };

    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_TENSOR_CONVERT_DEPTH, params, dimof(params));

    vxReleaseScalar(&overflow_policy_sc);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxTensorMatrixMultiplyNode(vx_graph graph, vx_tensor input1, vx_tensor input2, vx_tensor input3,
    const vx_tensor_matrix_multiply_params_t *matrix_multiply_params, vx_tensor output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar transpose_src1 = vxCreateScalar(context, VX_TYPE_BOOL, &matrix_multiply_params->transpose_input1);
    vx_scalar transpose_src2 = vxCreateScalar(context, VX_TYPE_BOOL, &matrix_multiply_params->transpose_input2);
    vx_scalar transpose_src3 = vxCreateScalar(context, VX_TYPE_BOOL, &matrix_multiply_params->transpose_input3);

    vx_reference params[] = {
        (vx_reference)input1,
        (vx_reference)input2,
        (vx_reference)input3,
        (vx_reference)transpose_src1,
        (vx_reference)transpose_src2,
        (vx_reference)transpose_src3,
        (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_TENSOR_MATRIX_MULTIPLY, params, dimof(params));

    vxReleaseScalar(&transpose_src1);
    vxReleaseScalar(&transpose_src2);
    vxReleaseScalar(&transpose_src3);
    return node;

}

VX_API_ENTRY vx_node VX_API_CALL vxMinNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
            (vx_reference) in1,
	    (vx_reference) in2,
            (vx_reference) out,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                    					   VX_KERNEL_MIN,
                                           params,
                                           dimof(params));
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxCopyNode(vx_graph graph, vx_reference input, vx_reference output)
{
    vx_reference params[] = {input, output};
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_COPY,
                                           params,
                                           dimof(params));
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxMaxNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
            (vx_reference) in1,
	    (vx_reference) in2,
            (vx_reference) out,
    };

    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_MAX,
                                           params,
                                           dimof(params));
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxLBPNode(vx_graph graph,
    vx_image in, vx_enum format, vx_int8 kernel_size, vx_image out)
{
    vx_scalar sformat = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_ENUM, &format);
    vx_scalar ksize = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_INT8, &kernel_size);
    vx_reference params[] = {
            (vx_reference)in,
            (vx_reference)sformat,
            (vx_reference)ksize,
            (vx_reference)out,
    };

    vx_node node = vxCreateNodeByStructure(graph,
        VX_KERNEL_LBP,
        params,
        dimof(params));

    vxReleaseScalar(&sformat);
    vxReleaseScalar(&ksize);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxBilateralFilterNode(vx_graph graph, vx_tensor src, vx_int32 diameter, vx_float32
    sigmaSpace, vx_float32 sigmaValues, vx_tensor dst)
{
    vx_scalar dia = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_INT32, &diameter);
    vx_scalar sSpa = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_FLOAT32, &sigmaSpace);
    vx_scalar sVal = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_FLOAT32, &sigmaValues);
    vx_reference params[] = {
            (vx_reference)src,
            (vx_reference)dia,
            (vx_reference)sSpa,
            (vx_reference)sVal,
            (vx_reference)dst,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_BILATERAL_FILTER,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&dia);
    vxReleaseScalar(&sSpa);
    vxReleaseScalar(&sVal);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxMatchTemplateNode(vx_graph graph, vx_image src, vx_image templateImage, vx_enum matchingMethod, vx_image output)
{
    vx_scalar scalar = vxCreateScalar(vxGetContext((vx_reference)graph), VX_TYPE_ENUM, &matchingMethod);
    vx_reference params[] = {
            (vx_reference)src,
            (vx_reference)templateImage,
            (vx_reference)scalar,
            (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_MATCH_TEMPLATE,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&scalar);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxScalarOperationNode(vx_graph graph, vx_enum operation, vx_scalar a, vx_scalar b, vx_scalar output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar op = vxCreateScalar(context, VX_TYPE_ENUM, &operation);
    vx_reference params[] = {
            (vx_reference)op,
            (vx_reference)a,
            (vx_reference)b,
            (vx_reference)output,
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_SCALAR_OPERATION,
                                           params,
                                           dimof(params));
    vxReleaseScalar(&op);
    return node;

}

VX_API_ENTRY vx_node VX_API_CALL vxSelectNode(vx_graph graph, vx_scalar condition, vx_reference true_value, vx_reference false_value, vx_reference output)
{
    vx_reference params[] = {
            (vx_reference)condition,
            (vx_reference)true_value,
            (vx_reference)false_value,
            (vx_reference)output
    };
    vx_node node = vxCreateNodeByStructure(graph,
                                           VX_KERNEL_SELECT,
                                           params,
                                           dimof(params));
    return node;
}

vx_node VX_API_CALL vxHOGCellsNode(vx_graph graph, vx_image input, vx_int32 cell_width, vx_int32 cell_height, vx_int32 num_bins, vx_tensor magnitudes, vx_tensor bins)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar cell_width_scalar = vxCreateScalar(context, VX_TYPE_INT32, &cell_width);
    vx_scalar cell_height_scalar = vxCreateScalar(context, VX_TYPE_INT32, &cell_height);
    vx_scalar num_bins_scalar = vxCreateScalar(context, VX_TYPE_INT32, &num_bins);

    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)cell_width_scalar,
        (vx_reference)cell_height_scalar,
        (vx_reference)num_bins_scalar,
        (vx_reference)magnitudes,
        (vx_reference)bins,
    };
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_HOG_CELLS, params, dimof(params));

    vxReleaseScalar(&cell_width_scalar);
    vxReleaseScalar(&cell_height_scalar);
    vxReleaseScalar(&num_bins_scalar);

    return node;
}

vx_node VX_API_CALL vxHOGFeaturesNode(vx_graph graph, vx_image input, vx_tensor magnitudes, vx_tensor bins, const vx_hog_t *params, vx_size hog_param_size, vx_tensor features)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_array hog_param = vxCreateArray(vxGetContext((vx_reference)graph), VX_TYPE_HOG_PARAMS, 1);
    vxAddArrayItems(hog_param, 1, params, hog_param_size*sizeof(vx_hog_t));
    vx_scalar hog_param_size_scalar = vxCreateScalar(context, VX_TYPE_INT32, &hog_param_size);

    vx_reference param[] = {
        (vx_reference)input,
        (vx_reference)magnitudes,
        (vx_reference)bins,
        (vx_reference)hog_param,
        (vx_reference)hog_param_size_scalar,
        (vx_reference)features,
    };
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_HOG_FEATURES, param, dimof(param));

    vxReleaseScalar(&hog_param_size_scalar);
    vxReleaseArray(&hog_param);

    return node;
}
