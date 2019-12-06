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

#ifndef _VX_C_MODEL_H_
#define _VX_C_MODEL_H_

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

#ifdef _MSC_VER
#define C_KERNEL_INLINE _inline
#else
#define C_KERNEL_INLINE inline
#endif

/*! \brief The largest convolution matrix the specification requires support for is 15x15.
 */
#define C_MAX_CONVOLUTION_DIM (15)

/*! \brief The largest nonlinear filter matrix the specification requires support for is 9x9.
*/
#define C_MAX_NONLINEAR_DIM (9)


#ifdef __cplusplus
extern "C" {
#endif

vx_status vxAbsDiff(vx_image in1, vx_image in2, vx_image output);

vx_status vxAccumulate(vx_image input, vx_image accum);
vx_status vxAccumulateWeighted(vx_image input, vx_scalar scalar, vx_image accum);
vx_status vxAccumulateSquare(vx_image input, vx_scalar scalar, vx_image accum);

vx_status vxAddition(vx_image in0, vx_image in1, vx_scalar policy_param, vx_image output);
vx_status vxSubtraction(vx_image in0, vx_image in1, vx_scalar policy_param, vx_image output);

vx_status vxAnd(vx_image in0, vx_image in1, vx_image output);
vx_status vxOr(vx_image in0, vx_image in1, vx_image output);
vx_status vxXor(vx_image in0, vx_image in1, vx_image output);
vx_status vxNot(vx_image input, vx_image output);

vx_status vxChannelCombine(vx_image inputs[4], vx_image output);
vx_status vxChannelExtract(vx_image src, vx_scalar channel, vx_image dst);

vx_status vxConvertColor(vx_image src, vx_image dst);
vx_status vxConvertDepth(vx_image input, vx_image output, vx_scalar spol, vx_scalar sshf);

vx_status vxConvolve(vx_image src, vx_convolution conv, vx_image dst, vx_border_t *bordermode);
vx_status vxConvolution3x3(vx_image src, vx_image dst, vx_int16 conv[3][3], const vx_border_t *borders);

vx_status vxFast9Corners(vx_image src, vx_scalar sens, vx_scalar nonm,
                         vx_array points, vx_scalar num_corners, vx_border_t *bordermode);

vx_status vxNonMaxSuppression(vx_image input, vx_image mask, vx_scalar win_size, vx_image output);
vx_status vxMedian3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxBox3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxGaussian3x3(vx_image src, vx_image dst, vx_border_t *bordermode);

vx_status vxNonLinearFilter(vx_scalar function, vx_image src, vx_matrix mask, vx_image dst, vx_border_t *bordermode);

vx_status vxHistogram(vx_image src, vx_distribution dist);
vx_status vxEqualizeHist(vx_image src, vx_image dst);

vx_status vxIntegralImage(vx_image src, vx_image dst);
vx_status vxTableLookup(vx_image src, vx_lut lut, vx_image dst);

vx_status vxMeanStdDev(vx_image input, vx_scalar mean, vx_scalar stddev);
vx_status vxMinMaxLoc(vx_image input, vx_scalar minVal, vx_scalar maxVal, vx_array minLoc, vx_array maxLoc, vx_scalar minCount, vx_scalar maxCount);
vx_status vxWeightedAverage(vx_image img1, vx_scalar alpha, vx_image img2, vx_image output);
vx_status vxErode3x3(vx_image src, vx_image dst, vx_border_t *bordermode);
vx_status vxDilate3x3(vx_image src, vx_image dst, vx_border_t *bordermode);

vx_status vxMagnitude(vx_image grad_x, vx_image grad_y, vx_image output);

vx_status vxMultiply(vx_image in0, vx_image in1, vx_scalar scale_param, vx_scalar opolicy_param, vx_scalar rpolicy_param, vx_image output);

vx_status vxOpticalFlowPyrLK(/*p1, p2, p3...*/);

vx_status vxPhase(vx_image grad_x, vx_image grad_y, vx_image output);

vx_status vxScaleImage(vx_image src_image, vx_image dst_image, vx_scalar stype, vx_border_t *bordermode, vx_float64 *interm, vx_size size);

vx_status vxSobel3x3(vx_image input, vx_image grad_x, vx_image grad_y, vx_border_t *bordermode);

vx_status vxThreshold(vx_image src_image, vx_threshold threshold, vx_image dst_image);

vx_status vxWarpPerspective(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders);
vx_status vxWarpAffine(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders);

vx_status vxLBP(vx_image src, vx_scalar sformat, vx_scalar ksize, vx_image dst);

vx_status vxMatchTemplate(vx_image src, vx_image template_image, vx_scalar matchingMethod, vx_image output);

vx_status vxCopy(vx_reference input, vx_reference output);

// Note: dims and strides point to the innner tensor impl arrays and therefore
//       it should always be legal to query any of 0..MAX_NUM_OF_DIMENSIONS-1
//       indexes in the kernels, regardless of dim_num. Unused dims should
//       have 0 strides - This allows for simpler sample kernels.
typedef struct {
    size_t dim_num;
    const size_t * dims;
    const size_t * strides;
} tensor_desc_t;


enum ElementwiseTensorMathOp
{
    ELEMENTWISE_TENSOR_ADD,
    ELEMENTWISE_TENSOR_SUB,
    ELEMENTWISE_TENSOR_MUL,
};

enum TensorCFmt
{
    TENSOR_C_FMT_Q78,
    TENSOR_C_FMT_U8,
    TENSOR_C_FMT_S8,
};

void ElementwiseTensorOpImpl(
        enum ElementwiseTensorMathOp op,
        enum TensorCFmt fmt,
        const void * input0_ptr, tensor_desc_t input0,
        const void * input1_ptr, tensor_desc_t input1,
        float scale,
        bool wrap,  // true for wrap, sat otherwise
        bool to_ne,  // true for to_ne, to_zero, otherwise (only usef for MUL)
        void * output_ptr, tensor_desc_t output);

vx_status TransposeTensorKernelImpl(
        vx_tensor in1,
        vx_scalar sc_dim1,
        vx_scalar sc_dim2,
        vx_tensor out,
        vx_size el_size);
vx_status vxTensorMultiply(vx_tensor in0, vx_tensor in1, vx_scalar scale_param, vx_scalar opolicy_param, vx_scalar rpolicy_param, vx_tensor output);
vx_status vxTensorMultiplyF16(vx_tensor in0, vx_tensor in1, vx_scalar scale_param, vx_scalar opolicy_param, vx_scalar rpolicy_param, vx_tensor output);
vx_status vxTensorAdd(vx_tensor in0, vx_tensor in1, vx_scalar policy_param, vx_tensor output);
vx_status vxTensorAddFP16(vx_tensor in0, vx_tensor in1, vx_scalar policy_param, vx_tensor output);
vx_status vxTensorSubtract(vx_tensor in0, vx_tensor in1, vx_scalar policy_param, vx_tensor output);
vx_status vxTensorSubtractFP16(vx_tensor in0, vx_tensor in1, vx_scalar policy_param, vx_tensor output);


void ConvolutionKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        const void * weight_ptr, tensor_desc_t weight,
        const void * bias_ptr, tensor_desc_t bias,
        size_t pad_x, size_t pad_y,
        size_t stride_x, size_t stride_y,
        bool wrap,  // true for WRAP, else SATURATE
        bool to_ne, // true for ROUND_TO_NE, else ROUND_TO_ZERO (only used for fmt == TT_MUL)
        size_t dilation_x, size_t dilation_y,
        void * output_ptr, tensor_desc_t output);

void SoftmaxKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        void * output_ptr, tensor_desc_t output);

void PoolingKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        bool max_pooling,   // MAX vs AVG pooling
        size_t size_x, size_t size_y,
        size_t pad_x, size_t pad_y,
        size_t stride_x, size_t stride_y,
        void * output_ptr, tensor_desc_t output);
void FullyConnectedKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        const void * weight_ptr, tensor_desc_t weight,
        const void * bias_ptr, tensor_desc_t bias,
        bool wrap,  // true for WRAP, else SATURATE
        bool to_ne, // true for ROUND_TO_NE, else ROUND_TO_ZERO (only used for fmt == TT_MUL)
        void * output_ptr, tensor_desc_t output);

void SampleSoftmaxQ78Kernel(
        const void * input_ptr, tensor_desc_t input,
        void * output_ptr, tensor_desc_t output);

vx_status NNNormalizationKernelImpl(vx_tensor inputs, vx_scalar type_scalar, vx_scalar norm_size_scalar, vx_scalar alpha_scalar, vx_scalar beta_scalar, vx_tensor outputs);

void ROIPoolingKernelImpl(
        enum TensorCFmt fmt,
        const void * in0_ptr, tensor_desc_t in0,
        const void * in1_ptr, tensor_desc_t in1,
        void * output_ptr, tensor_desc_t out);

void DeconvolutionKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        const void * weight_ptr, tensor_desc_t weight,
        const void * bias_ptr, tensor_desc_t bias,
        vx_size pad_x, vx_size pad_y,
        vx_size upscale_x, vx_size upscale_y,
        bool wrap,  // true for WRAP, else SATURATE
        bool to_ne, // true for ROUND_TO_NE, else ROUND_TO_ZERO
        vx_size a_x, vx_size a_y,
        void * output_ptr, tensor_desc_t output);

vx_status vxTensorTableLookup(
        void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims, void* lut, vx_size lut_size,
        vx_uint32 lut_offset, void* dst, vx_size* dst_strides, vx_enum type);

void Multiply2DMatrixesImpl(
        const void* src1, const vx_size* src1_strides, const vx_size* dims1,
        const void* src2, const vx_size* src2_strides, const vx_size* dims2,
        const void* src3, const vx_size* src3_strides,
        void* dst, const vx_size* dst_strides,
        vx_enum type);

void TensorConvertDepthKernelImpl(
        const void * input_ptr, tensor_desc_t input,
        enum TensorCFmt src_fmt,
        enum TensorCFmt dst_fmt,
        bool wrap,  // true for wrap, sat otherwise
        float norm,
        float offset,
        void * output_ptr, tensor_desc_t output);

vx_status vxMin(vx_image in0, vx_image in1, vx_image output);

vx_status vxMax(vx_image in0, vx_image in1, vx_image output);

void ActivationKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        vx_enum func,
        float a, float b,
        void * output_ptr, tensor_desc_t output);

vx_status vxBilateralFilter(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
        vx_int32 diameter, vx_float32 sigmaSpace, vx_float32 sigmaValues,
        void* dst, vx_size* dst_strides, vx_enum type, vx_border_t *bordermode);

vx_status vxHoughLinesP(vx_image img, vx_array param_hough_lines_array, vx_array lines_array, vx_scalar num_lines);

vx_status vxScalarOperation(vx_scalar scalar_operation, vx_scalar a, vx_scalar b, vx_scalar output);
vx_status vxSelect(vx_scalar condition, vx_reference true_value, vx_reference false_value, vx_reference output);

vx_status vxHogCells(vx_image img, vx_scalar cell_width, vx_scalar cell_height, vx_scalar num_bins, vx_tensor magnitudes, vx_tensor bins);
vx_status vxHogFeatures(vx_image img, vx_tensor magnitudes, vx_tensor bins, vx_array hog_params, vx_scalar hog_param_size, vx_tensor features);

#ifdef __cplusplus
}
#endif

#endif
