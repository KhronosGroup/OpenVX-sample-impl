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

#ifndef _VX_VENUM_H_
#define _VX_VENUM_H_

#include <VX/vx.h>
#include <VX/vx_helper.h>
/* TODO: remove vx_compatibility.h after transition period */
#include <VX/vx_compatibility.h>
#include <VX/vx_khr_nn.h>

#include <math.h>
#include <stdbool.h>

//TODO(ruv): temp, remove me
#define USE_NEW_SAMPLE_CONV_KERNEL_FOR_Q78
//#define USE_NEW_SAMPLE_FULLYCONNECTED_KERNEL_FOR_Q78
#define USE_NEW_SAMPLE_POOL_KERNEL_FOR_Q78
#define USE_NEW_SAMPLE_SOFTMAX_KERNEL_FOR_Q78
//#define USE_NEW_SAMPLE_NORM_KERNEL_FOR_Q78
//#define USE_NEW_SAMPLE_ACTIVATION_KERNEL_FOR_Q78
#define USE_NEW_SAMPLE_ROIPOOL_KERNEL
//#define USE_NEW_SAMPLE_DECONV_KERNEL_FOR_Q78

//TODO(ruv): temp, remove me
#define HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC

/*! \brief The largest convolution matrix the specification requires support for is 15x15.
 */
#define VENUM_MAX_CONVOLUTION_DIM (15)

/*! \brief The largest nonlinear filter matrix the specification requires support for is 9x9.
*/
#define VENUM_MAX_NONLINEAR_DIM (9)


#ifdef __cplusplus
extern "C" {
#endif

vx_status vxAbsDiff(vx_image in1, vx_image in2, vx_image output);
vx_status vxThreshold(vx_image src_image, vx_threshold threshold, vx_image dst_image);
vx_status vxPhase(vx_image grad_x, vx_image grad_y, vx_image output);
vx_status vxNonLinearFilter(vx_scalar function, vx_image src, vx_matrix mask, vx_image dst, vx_border_t *border);
vx_status vxConvertColor(vx_image src, vx_image dst);
vx_status vxAddition(vx_image in0, vx_image in1, vx_scalar policy_param, vx_image output);
vx_status vxSubtraction(vx_image in0, vx_image in1, vx_scalar policy_param, vx_image output);
vx_status vxMedian3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxErode3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxDilate3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxBox3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxSobel3x3(vx_image input, vx_image grad_x, vx_image grad_y, vx_border_t *bordermode);
vx_status vxNot(vx_image input, vx_image output);
vx_status vxAnd(vx_image in0, vx_image in1, vx_image output);
vx_status vxOr(vx_image in0, vx_image in1, vx_image output);
vx_status vxXor(vx_image in0, vx_image in1, vx_image output);
vx_status vxGaussian3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxMultiply(vx_image in0, vx_image in1, vx_scalar scale_param, vx_scalar opolicy_param, vx_scalar rpolicy_param, vx_image output);
vx_status vxChannelExtract(vx_image src, vx_scalar channel, vx_image dst);
vx_status vxHistogram(vx_image src, vx_distribution dist);
vx_status vxMagnitude(vx_image grad_x, vx_image grad_y, vx_image output);
vx_status vxConvertDepth(vx_image input, vx_image output, vx_scalar spol, vx_scalar sshf);
vx_status vxWarpPerspective(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders);
vx_status vxWarpAffine(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders);
vx_status vxCopy(vx_reference input, vx_reference output);
vx_status vxGaussian5x5(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxScaleImage(vx_image src_image, vx_image dst_image, vx_scalar stype, vx_border_t *bordermode, vx_float64 *interm, vx_size size);
vx_status vxHogFeatures(vx_image img, void * magnitudes, void * bins, vx_array hog_params, vx_scalar hog_param_size, void * features);
vx_status vxHogCells(vx_image img, vx_scalar cell_width, vx_scalar cell_height, vx_scalar num_bins, vx_tensor magnitudes, vx_tensor bins);
vx_status vxHoughLinesP(vx_image img, vx_array param_hough_lines_array, vx_array lines_array, vx_scalar num_lines);
vx_status vxNonMaxSuppression(vx_image input, vx_image mask, vx_scalar win_size, vx_image output);
vx_status vxEqualizeHist(vx_image src, vx_image dst);
vx_status vxIntegralImage(vx_image src, vx_image dst);
vx_status vxMeanStdDev(vx_image input, vx_scalar mean, vx_scalar stddev);
vx_status vxMin(vx_image in0, vx_image in1, vx_image output);
vx_status vxMax(vx_image in0, vx_image in1, vx_image output);
vx_status vxMatchTemplate(vx_image src, vx_image template_image, vx_scalar matchingMethod, vx_image output);

void Multiply2DMatrixesImpl(
        const void* src1, const vx_size* src1_strides, const vx_size* dims1,
        const void* src2, const vx_size* src2_strides, const vx_size* dims2,
        const void* src3, const vx_size* src3_strides,
        void* dst, const vx_size* dst_strides,
        vx_enum type);


#ifdef __cplusplus
}
#endif

#endif
