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

#ifndef _OPENVX_INTERFACE_H_
#define _OPENVX_INTERFACE_H_

#include <c_model.h>
#include <VX/vx_helper.h>

extern vx_kernel_description_t colorconvert_kernel;
extern vx_kernel_description_t channelextract_kernel;
extern vx_kernel_description_t sobel3x3_kernel;
extern vx_kernel_description_t magnitude_kernel;
extern vx_kernel_description_t phase_kernel;
extern vx_kernel_description_t channelcombine_kernel;
extern vx_kernel_description_t scale_image_kernel;
extern vx_kernel_description_t lut_kernel;
extern vx_kernel_description_t histogram_kernel;
extern vx_kernel_description_t equalize_hist_kernel;
extern vx_kernel_description_t absdiff_kernel;
extern vx_kernel_description_t mean_stddev_kernel;
extern vx_kernel_description_t threshold_kernel;
extern vx_kernel_description_t integral_image_kernel;
extern vx_kernel_description_t erode3x3_kernel;
extern vx_kernel_description_t dilate3x3_kernel;
extern vx_kernel_description_t median3x3_kernel;
extern vx_kernel_description_t box3x3_kernel;
extern vx_kernel_description_t box3x3_kernel_2;
extern vx_kernel_description_t gaussian3x3_kernel;
extern vx_kernel_description_t laplacian3x3_kernel;
extern vx_kernel_description_t convolution_kernel;
extern vx_kernel_description_t gaussian_pyramid_kernel;
extern vx_kernel_description_t accumulate_kernel;
extern vx_kernel_description_t accumulate_weighted_kernel;
extern vx_kernel_description_t accumulate_square_kernel;
extern vx_kernel_description_t minmaxloc_kernel;
extern vx_kernel_description_t weightedaverage_kernel;
extern vx_kernel_description_t convertdepth_kernel;
extern vx_kernel_description_t canny_kernel;
extern vx_kernel_description_t scharr3x3_kernel;
extern vx_kernel_description_t and_kernel;
extern vx_kernel_description_t or_kernel;
extern vx_kernel_description_t xor_kernel;
extern vx_kernel_description_t not_kernel;
extern vx_kernel_description_t multiply_kernel;
extern vx_kernel_description_t add_kernel;
extern vx_kernel_description_t subtract_kernel;
extern vx_kernel_description_t warp_affine_kernel;
extern vx_kernel_description_t warp_perspective_kernel;
extern vx_kernel_description_t harris_kernel;
extern vx_kernel_description_t fast9_kernel;
extern vx_kernel_description_t nonmaxsuppression_kernel;
extern vx_kernel_description_t optpyrlk_kernel;
extern vx_kernel_description_t remap_kernel;
extern vx_kernel_description_t halfscale_gaussian_kernel;
extern vx_kernel_description_t laplacian_pyramid_kernel;
extern vx_kernel_description_t laplacian_reconstruct_kernel;
extern vx_kernel_description_t nonlinearfilter_kernel;

extern vx_kernel_description_t tensor_multiply_kernel;
extern vx_kernel_description_t tensor_add_kernel;
extern vx_kernel_description_t tensor_subtract_kernel;
extern vx_kernel_description_t tensor_lut_kernel;
extern vx_kernel_description_t tensor_transpose_kernel;
extern vx_kernel_description_t tensor_convert_depth_kernel;
extern vx_kernel_description_t tensor_matrix_multiply_kernel;
extern vx_kernel_description_t lbp_kernel;

extern vx_kernel_description_t bilateral_filter_kernel;
extern vx_kernel_description_t min_kernel;
extern vx_kernel_description_t max_kernel;
extern vx_kernel_description_t match_template_kernel;
extern vx_kernel_description_t houghlinesp_kernel;
extern vx_kernel_description_t copy_kernel;
extern vx_kernel_description_t scalar_operation_kernel;
extern vx_kernel_description_t select_kernel;
extern vx_kernel_description_t hogcells_kernel;
extern vx_kernel_description_t hogfeatures_kernel;

#ifdef OPENVX_USE_NN
extern vx_kernel_description_t nn_convolution_kernel;
extern vx_kernel_description_t nn_deconvolution_kernel;
extern vx_kernel_description_t nn_pooling_kernel;
extern vx_kernel_description_t nn_fully_connected_kernel;
extern vx_kernel_description_t nn_softmax_kernel;
extern vx_kernel_description_t nn_norm_kernel;
extern vx_kernel_description_t nn_activation_kernel;
extern vx_kernel_description_t nn_roipooling_kernel;
#endif


#endif

