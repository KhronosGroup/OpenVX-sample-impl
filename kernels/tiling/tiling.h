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

#ifndef _TILING_H_
#define _TILING_H_

#include <VX/vx_khr_tiling.h>
#include <VX/vx_helper.h>

/*! \brief The tile structure declaration.
 * \ingroup group_tiling
 */
typedef struct _vx_tile_ex_t {
    /*! \brief The array of pointers to the tile's image plane. */
    vx_uint8 * VX_RESTRICT base[VX_MAX_TILING_PLANES];
    /*! \brief The top left X pixel index within the width dimension of the image. */
    vx_uint32 tile_x;
    /*! \brief The top left Y pixel index within the height dimension of the image. */
    vx_uint32 tile_y;
    /*! \brief The array of addressing structure to describe each plane. */
    vx_imagepatch_addressing_t addr[VX_MAX_TILING_PLANES];
    /*! \brief The output block size structure. */
    vx_tile_block_size_t tile_block;
    /*! \brief The neighborhood definition. */
    vx_neighborhood_size_t neighborhood;
    /*! \brief The description and attributes of the image. */
    vx_image_description_t image;
    /*! \brief rectangle for U1. */
    vx_rectangle_t rect;
    /*! \brief flag for U1. */
    vx_bool is_U1;
    /*! border information. */
    vx_border_t border;
    /*! image null information. */
    vx_bool is_Null;
} vx_tile_ex_t;

 // tag::publish_support[]
typedef struct _vx_tiling_kernel_t {
    /*! kernel name */
    vx_char name[VX_MAX_KERNEL_NAME];
    /*! kernel enum */
    vx_enum enumeration;
    /*! \brief The pointer to the function to execute the kernel */
    vx_kernel_f function;
    /*! tiling flexible function pointer */
    vx_tiling_kernel_f flexible_function;
    /*! tiling fast function pointer */
    vx_tiling_kernel_f fast_function;
    /*! number of parameters */
    vx_uint32 num_params;
    /*! set of parameters */
    vx_param_description_t parameters[10];
    /* validate */
    vx_kernel_validate_f validate;
    /*! input validator */
    void *input_validator;
    /*! output validator */
    void *output_validator;
    /*! \brief The initialization function */
    vx_kernel_initialize_f initialize;
    /*! \brief The deinitialization function */
    vx_kernel_deinitialize_f deinitialize;
    /*! block size */
    vx_tile_block_size_t block;
    /*! neighborhood around block */
    vx_neighborhood_size_t nbhd;
    /*! border information. */
    vx_border_t border;
} vx_tiling_kernel_t;


typedef struct _vx_tile_threshold {
    /*! \brief From \ref vx_threshold_type_e */
    vx_enum thresh_type;
    /*! \brief The binary threshold value */
    vx_pixel_value_t value;
    /*! \brief Lower bound for range threshold */
    vx_pixel_value_t lower;
    /*! \brief Upper bound for range threshold */
    vx_pixel_value_t upper;
    /*! \brief True value for output */
    vx_pixel_value_t true_value;
    /*! \brief False value for output */
    vx_pixel_value_t false_value;
    /*! \brief The input image format */
    vx_df_image input_format;
    /*! \brief The input image format */
    vx_df_image output_format;
} vx_tile_threshold_t;


#define C_MAX_NONLINEAR_DIM (9)

typedef struct _vx_tile_matrix {
    /*! \brief From \ref vx_type_e */
    vx_enum data_type;
    /*! \brief Number of columns */
    vx_size columns;
    /*! \brief Number of rows */
    vx_size rows;
    /*! \brief Origin */
    vx_coordinates2d_t origin;
    /*! \brief Pattern */
    vx_enum pattern;
    /*! \brief Mask */
    vx_uint8 m[C_MAX_NONLINEAR_DIM * C_MAX_NONLINEAR_DIM];
    /*! \brief Mask float32 */
    vx_float32 m_f32[C_MAX_NONLINEAR_DIM];
} vx_tile_matrix_t;


#define C_MAX_CONVOLUTION_DIM (15)

typedef struct _vx_tile_convolution {
    vx_size conv_width;
    vx_size conv_height;
    vx_int16 conv_mat[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM];
    vx_uint32 scale;
} vx_tile_convolution_t;

typedef struct _vx_tile_array {
    void *ptr;
    /*! \brief The item type of the array. */
    vx_enum item_type;
    /*! \brief The size of array item in bytes */
    vx_size item_size;
    /*! \brief The number of items in the array */
    vx_size num_items;
    /*! \brief The array capacity */
    vx_size capacity;
    /*! \brief Offset attribute value. Used internally by LUT implementation */
    vx_uint32 offset;
} vx_tile_array_t;

#ifdef  __cplusplus
extern "C" {
#endif

/*! \brief Allows a user to add a tile-able kernel to the OpenVX system.
 * \param [in] context The handle to the implementation context.
 * \param [in] name The string to be used to match the kernel.
 * \param [in] enumeration The enumerated value of the kernel to be used by clients.
 * \param [in] flexible_func_ptr The process-local flexible function pointer to be invoked.
 * \param [in] fast_func_ptr The process-local fast function pointer to be invoked.
 * \param [in] num_params The number of parameters for this kernel.
 * \param [in] input The pointer to a function which will validate the
 * input parameters to this kernel.
 * \param [in] output The pointer to a function which will validate the
 * output parameters to this kernel.
 * \note Tiling Kernels do not have access to any of the normal node attributes listed
 * in \ref vx_node_attribute_e.
 * \post Call <tt>\ref vxAddParameterToKernel</tt> for as many parameters as the function has,
 * then call <tt>\ref vxFinalizeKernel</tt>.
 * \retval 0 Indicates that an error occurred when adding the kernel.
 * Note that the fast or flexible formula, but not both, can be NULL.
 * \ingroup group_tiling
 */
VX_API_ENTRY vx_kernel VX_API_CALL vxAddTilingKernelEx(vx_context context,
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
                            vx_kernel_deinitialize_f deinitialize);

#ifdef  __cplusplus
}
#endif

#endif

void box3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void box3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Phase_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Phase_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void And_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void And_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Or_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Or_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Xor_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Xor_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Not_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Not_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Threshold_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Threshold_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void ConvertColor_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void ConvertColor_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Multiply_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Multiply_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void NonLinearFilter_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void NonLinearFilter_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Magnitude_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Magnitude_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Erode3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Erode3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Dilate3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Dilate3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Median3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Median3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Sobel3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Sobel3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Max_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Max_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Min_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Min_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Gaussian3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Gaussian3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Addition_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Addition_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Subtraction_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Subtraction_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void ConvertDepth_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void ConvertDepth_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void WarpAffine_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void WarpAffine_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void WarpPerspective_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void WarpPerspective_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void WeightedAverage_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void WeightedAverage_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void AbsDiff_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void AbsDiff_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void IntegralImage_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void IntegralImage_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Convolve_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Convolve_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void HogFeatures_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void HogFeatures_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void Fast9Corners_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void Fast9Corners_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void LBP_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void LBP_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void ScaleImage_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void ScaleImage_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void TableLookup_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void TableLookup_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void ChannelCombine_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void ChannelCombine_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void NonMaxSuppression_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void NonMaxSuppression_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void HogCells_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void HogCells_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);

void HoughLinesP_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void HoughLinesP_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
