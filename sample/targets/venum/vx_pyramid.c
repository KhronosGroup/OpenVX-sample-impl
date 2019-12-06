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
 * \brief The Image Pyramid Kernel
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>
#include <venum.h>
#include <VX/vxu.h>
#include "vx_internal.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <arm_neon.h>

static const vx_uint32 gaussian5x5scale = 256;
static const vx_int16 gaussian5x5[5][5] =
{
    {1,  4,  6,  4, 1},
    {4, 16, 24, 16, 4},
    {6, 24, 36, 24, 6},
    {4, 16, 24, 16, 4},
    {1,  4,  6,  4, 1}
};

static vx_param_description_t gaussian_pyramid_kernel_params[] =
{
    {VX_INPUT,  VX_TYPE_IMAGE,   VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_PYRAMID, VX_PARAMETER_STATE_REQUIRED},
};

static vx_convolution vxCreateGaussian5x5Convolution(vx_context context)
{
    vx_convolution conv = vxCreateConvolution(context, 5, 5);
    vx_status status = vxCopyConvolutionCoefficients(conv, (vx_int16 *)gaussian5x5, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    if (status != VX_SUCCESS)
    {
        vxReleaseConvolution(&conv);
        return NULL;
    }

    status = vxSetConvolutionAttribute(conv, VX_CONVOLUTION_SCALE, (void *)&gaussian5x5scale, sizeof(vx_uint32));
    if (status != VX_SUCCESS)
    {
        vxReleaseConvolution(&conv);
        return NULL;
    }
    return conv;
}

static vx_status ownCopyImage(vx_image input, vx_image output)
{
    vx_status status = VX_SUCCESS; // assume success until an error occurs.
    vx_uint32 p = 0;
    vx_uint32 y = 0, x = 0;
    vx_size planes = 0;

    void* src;
    void* dst;
    vx_imagepatch_addressing_t src_addr;
    vx_imagepatch_addressing_t dst_addr;
    vx_rectangle_t src_rect, dst_rect;
    vx_map_id map_id1;
    vx_map_id map_id2;
    vx_df_image src_format = 0;
    vx_df_image out_format = 0;

    status |= vxQueryImage(input, VX_IMAGE_PLANES, &planes, sizeof(planes));
    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(input, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxGetValidRegionImage(input, &src_rect);
    status |= vxGetValidRegionImage(output, &dst_rect);
    for (p = 0; p < planes && status == VX_SUCCESS; p++)
    {
        status = VX_SUCCESS;
        src = NULL;
        dst = NULL;

        status |= vxMapImagePatch(input, &src_rect, p, &map_id1, &src_addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        status |= vxMapImagePatch(output, &dst_rect, p, &map_id2, &dst_addr, &dst, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

        for (y = 0; y < src_addr.dim_y && status == VX_SUCCESS; y += src_addr.step_y)
        {
            for (x = 0; x < src_addr.dim_x && status == VX_SUCCESS; x += src_addr.step_x)
            {
                void* srcp = vxFormatImagePatchAddress2d(src, x, y, &src_addr);
                void* dstp = vxFormatImagePatchAddress2d(dst, x, y, &dst_addr);
                vx_int32 out0 = src_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)srcp : *(vx_int16 *)srcp;

                if (out_format == VX_DF_IMAGE_U8)
                {
                    if (out0 > UINT8_MAX)
                        out0 = UINT8_MAX;
                    else if (out0 < 0)
                        out0 = 0;
                    *(vx_uint8 *)dstp = (vx_uint8)out0;
                }
                else
                {
                    if (out0 > INT16_MAX)
                        out0 = INT16_MAX;
                    else if (out0 < INT16_MIN)
                        out0 = INT16_MIN;
                    *(vx_int16 *)dstp = (vx_int16)out0;
                }
            }
        }

        if (status == VX_SUCCESS)
        {
            status |= vxUnmapImagePatch(input, map_id1);
            status |= vxUnmapImagePatch(output, map_id2);
        }
    }

    return status;
}

static vx_status ownCopyImage_U8(vx_image input, vx_image output)
{
    vx_status status = VX_SUCCESS; // assume success until an error occurs.
    vx_uint32 p = 0;
    vx_uint32 y = 0, x = 0;
    vx_size planes = 0;

    void* src;
    void* dst;
    vx_imagepatch_addressing_t src_addr;
    vx_imagepatch_addressing_t dst_addr;
    vx_rectangle_t src_rect, dst_rect;
    vx_map_id map_id1;
    vx_map_id map_id2;
    vx_df_image src_format = 0;
    vx_df_image out_format = 0;

    status |= vxQueryImage(input, VX_IMAGE_PLANES, &planes, sizeof(planes));
    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(input, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxGetValidRegionImage(input, &src_rect);
    status |= vxGetValidRegionImage(output, &dst_rect);

    vx_uint32 wWidth;
    for (p = 0; p < planes && status == VX_SUCCESS; p++)
    {
        status = VX_SUCCESS;

        src = NULL;
        dst = NULL;

        status |= vxMapImagePatch(input, &src_rect, p, &map_id1, &src_addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        status |= vxMapImagePatch(output, &dst_rect, p, &map_id2, &dst_addr, &dst, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

        wWidth = (src_addr.dim_x >> 4) << 4;
        for (y = 0; y < src_addr.dim_y && status == VX_SUCCESS; y += dst_addr.step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src + y * src_addr.stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr.stride_y;
            for (x = 0; x < wWidth; x += 16)
            {
                uint8x16_t vSrc = vld1q_u8(ptr_src + x * src_addr.stride_x);
                vst1q_u8(ptr_dst + x, vSrc);
            }
            for (x = wWidth; x < src_addr.dim_x; x += src_addr.step_x)
            {
                void* srcp = vxFormatImagePatchAddress2d(src, x, y, &src_addr);
                vx_int32 out0 = src_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)srcp : *(vx_int16 *)srcp;
                ptr_dst[x] = out0;
            }
        }

        if (status == VX_SUCCESS)
        {
            status |= vxUnmapImagePatch(input, map_id1);
            status |= vxUnmapImagePatch(output, map_id2);
        }
    }

    return status;
}

static vx_status VX_CALLBACK vxGaussianPyramidKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (num == dimof(gaussian_pyramid_kernel_params))
    {
        status = VX_SUCCESS;
    }

    return status;
}


static vx_status VX_CALLBACK vxGaussianPyramidInputValidator(vx_node node, vx_uint32 index)
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
    return status;
}


static vx_status VX_CALLBACK vxGaussianPyramidOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_image src = 0;
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);

        vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
        if (src)
        {
            vx_pyramid dst = 0;
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));

            if (dst)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format;
                vx_size num_levels;
                vx_float32 scale;

                vxQueryImage(src, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &height, sizeof(height));
                vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryPyramid(dst, VX_PYRAMID_LEVELS, &num_levels, sizeof(num_levels));
                vxQueryPyramid(dst, VX_PYRAMID_SCALE, &scale, sizeof(scale));

                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_PYRAMID;
                ptr->dim.pyramid.width = width;
                ptr->dim.pyramid.height = height;
                ptr->dim.pyramid.format = format;
                ptr->dim.pyramid.levels = num_levels;
                ptr->dim.pyramid.scale = scale;
                status = VX_SUCCESS;
                vxReleasePyramid(&dst);
            }
            vxReleaseImage(&src);
        }
        vxReleaseParameter(&dst_param);
        vxReleaseParameter(&src_param);
    }
    return status;
}


static vx_status VX_CALLBACK vxGaussianPyramidInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_SUCCESS;

    vx_context context = vxGetContext((vx_reference)node);

    vx_uint32 mid_width = 0;
    vx_uint32 mid_height = 0;

    vx_size lev;
    vx_size levels = 1;

    vx_enum interp = VX_INTERPOLATION_NEAREST_NEIGHBOR;

    vx_image input = (vx_image)parameters[0];
    vx_pyramid gaussian = (vx_pyramid)parameters[1];

    status |= vxQueryPyramid(gaussian, VX_PYRAMID_LEVELS, &levels, sizeof(levels));

    vx_image level0 = vxGetPyramidLevel(gaussian, 0);

    ownCopyImage_U8(input, level0);

    status |= vxReleaseImage(&level0);

    vx_border_t bordermode;
    status = vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));

    bordermode.mode = VX_BORDER_REPLICATE;

    vx_enum policy = VX_INTERPOLATION_NEAREST_NEIGHBOR;

    vx_scalar stype = vxCreateScalar(context, VX_TYPE_ENUM, &policy);

    vx_size sizek = 1;

    vx_float64 size_f64 = (vx_float64)sizek;

    vx_float64 *interm = &size_f64;

    for (lev = 1; lev < levels; lev++)
    {
        vx_image tmp0 = vxGetPyramidLevel(gaussian, (vx_uint32)lev - 1);
        vx_image tmp1 = vxGetPyramidLevel(gaussian, (vx_uint32)lev);

        vxQueryImage(tmp0, VX_IMAGE_WIDTH, &mid_width, sizeof(mid_width));
        vxQueryImage(tmp0, VX_IMAGE_HEIGHT, &mid_height, sizeof(mid_height));

        // Use to store the dst of gaussian5x5 and as the input in scaling
        vx_image mid_dst = vxCreateImage(context, mid_width, mid_height, VX_DF_IMAGE_U8);

        vxGaussian5x5(tmp0, mid_dst, &bordermode);
        vxScaleImage(mid_dst, tmp1, stype, &bordermode, interm, sizek);

        /* decrements the references */
        status |= vxReleaseImage(&tmp0);
        status |= vxReleaseImage(&tmp1);
        status |= vxReleaseImage(&mid_dst);
    }

    status |= vxReleaseScalar(&stype);

    return status;
}


static vx_status VX_CALLBACK vxGaussianPyramidDeinitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (num == dimof(gaussian_pyramid_kernel_params))
    {
        status = VX_SUCCESS;
    }

    return status;
}


vx_kernel_description_t gaussian_pyramid_kernel =
{
    VX_KERNEL_GAUSSIAN_PYRAMID,
    "org.khronos.openvx.gaussian_pyramid",
    vxGaussianPyramidKernel,
    gaussian_pyramid_kernel_params, dimof(gaussian_pyramid_kernel_params),
    NULL,
    vxGaussianPyramidInputValidator,
    vxGaussianPyramidOutputValidator,
    vxGaussianPyramidInitializer,
    vxGaussianPyramidDeinitializer,
};


static void auxReadRect(const void *base, const vx_imagepatch_addressing_t *addr, const vx_border_t *borders, vx_df_image type,
    vx_uint32 center_x, vx_uint32 center_y, vx_uint32 radius_x, vx_uint32 radius_y, void *destination)
{
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 dest_index = 0;
    // kx, kx - kernel x and y
    if (borders->mode == VX_BORDER_REPLICATE || borders->mode == VX_BORDER_UNDEFINED)
    {
        for (ky = -(int32_t)radius_y; ky <= (int32_t)radius_y; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            y = y < 0 ? 0 : (y >= height ? height - 1 : y);

            for (kx = -(int32_t)radius_x; kx <= (int32_t)radius_x; ++kx, ++dest_index)
            {
                vx_int32 x = (int32_t)(center_x + kx);
                x = x < 0 ? 0 : (x >= width ? width - 1 : x);

                switch (type)
                {
                case VX_DF_IMAGE_U8:
                    ((vx_uint8*)destination)[dest_index] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
                    break;
                case VX_DF_IMAGE_S16:
                case VX_DF_IMAGE_U16:
                    ((vx_uint16*)destination)[dest_index] = *(vx_uint16*)(ptr + y*stride_y + x*stride_x);
                    break;
                case VX_DF_IMAGE_S32:
                case VX_DF_IMAGE_U32:
                    ((vx_uint32*)destination)[dest_index] = *(vx_uint32*)(ptr + y*stride_y + x*stride_x);
                    break;
                default:
                    abort();
                }
            }
        }
    }
    else if (borders->mode == VX_BORDER_CONSTANT)
    {
        vx_pixel_value_t cval = borders->constant_value;
        for (ky = -(int32_t)radius_y; ky <= (int32_t)radius_y; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            int ccase_y = y < 0 || y >= height;

            for (kx = -(int32_t)radius_x; kx <= (int32_t)radius_x; ++kx, ++dest_index)
            {
                vx_int32 x = (int32_t)(center_x + kx);
                int ccase = ccase_y || x < 0 || x >= width;

                switch (type)
                {
                case VX_DF_IMAGE_U8:
                    if (!ccase)
                        ((vx_uint8*)destination)[dest_index] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
                    else
                        ((vx_uint8*)destination)[dest_index] = (vx_uint8)cval.U8;
                    break;
                case VX_DF_IMAGE_S16:
                case VX_DF_IMAGE_U16:
                    if (!ccase)
                        ((vx_uint16*)destination)[dest_index] = *(vx_uint16*)(ptr + y*stride_y + x*stride_x);
                    else
                        ((vx_uint16*)destination)[dest_index] = (vx_uint16)cval.U16;
                    break;
                case VX_DF_IMAGE_S32:
                case VX_DF_IMAGE_U32:
                    if (!ccase)
                        ((vx_uint32*)destination)[dest_index] = *(vx_uint32*)(ptr + y*stride_y + x*stride_x);
                    else
                        ((vx_uint32*)destination)[dest_index] = (vx_uint32)cval.U32;
                    break;
                default:
                    abort();
                }
            }
        }
    }
    else
        abort();
}

#define CONV_DIM 5
#define CONV_DIM_HALF CONV_DIM / 2
#define C_MAX_CONVOLUTION_DIM (15)

#define INSERT_ZERO_Y(slice, y) for (int i=0; i<CONV_DIM; i++) slice[CONV_DIM*(1-y)+i] = 0;
#define INSERT_VALUES_Y(slice, y) for (int i=0; i<CONV_DIM; i++) slice[CONV_DIM*(high_y-y)+i+CONV_DIM_HALF*CONV_DIM] = slice[CONV_DIM*(high_y-y)+i];
#define INSERT_ZERO_X(slice, x) for (int i=0; i<CONV_DIM; i++) slice[CONV_DIM*i+1-x] = 0;
#define INSERT_VALUES_X(slice, x) for (int i=0; i<CONV_DIM; i++) slice[CONV_DIM*i+(high_x-x)+CONV_DIM_HALF] = slice[CONV_DIM*i+(high_x-x)];


static vx_status replicateConvolve(vx_image src, vx_convolution conv, vx_image dst, vx_border_t *bordermode)
{
    vx_int32 y, x, i;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;

    vx_size conv_width, conv_height;
    vx_int32 conv_radius_x, conv_radius_y;
    vx_int16 conv_mat[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };
    vx_int32 sum = 0, value = 0;
    vx_uint32 scale = 1;
    vx_df_image src_format = 0;
    vx_df_image dst_format = 0;
    vx_status status = VX_SUCCESS;
    vx_int32 low_x, low_y, high_x, high_y;

    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_COLUMNS, &conv_width, sizeof(conv_width));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_ROWS, &conv_height, sizeof(conv_height));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_SCALE, &scale, sizeof(scale));
    conv_radius_x = (vx_int32)conv_width / 2;
    conv_radius_y = (vx_int32)conv_height / 2;
    status |= vxCopyConvolutionCoefficients(conv, conv_mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    low_x = 0;
    high_x = src_addr.dim_x;
    low_y = 0;
    high_y = src_addr.dim_y;

    vx_int32 roiw8 = conv_width * conv_height >= 7 ? conv_width * conv_height - 7 : 0;
    for (y = low_y; y < high_y; ++y)
    {
        for (x = low_x; x < high_x; ++x)
        {
            sum = 0;

            if (src_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 slice[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };
                auxReadRect(src_base, &src_addr, bordermode, src_format, x, y, conv_radius_x, conv_radius_y, slice);

                if (y < CONV_DIM_HALF)
                {
                    INSERT_ZERO_Y(slice, y)
                }
                else if (y >= high_y - CONV_DIM_HALF)
                {
                    INSERT_VALUES_Y(slice, y)
                }

                if (x < CONV_DIM_HALF)
                {
                    INSERT_ZERO_X(slice, x)
                }
                else if (x >= high_x - CONV_DIM_HALF)
                {
                    INSERT_VALUES_X(slice, x)
                }
                i = 0;
                for(; i < roiw8; i+=8)
                {
                    int16x8_t conv_s16x8 = vld1q_s16(&conv_mat[i]);
                    uint8x8_t slice_u8x8 = vld1_u8(&slice[i]);
                    int16x8_t sum_s16x8 = vmulq_s16(conv_s16x8, vreinterpretq_s16_u16(vmovl_u8(slice_u8x8)));
                    sum += (vgetq_lane_s16(sum_s16x8, 0)
                            +vgetq_lane_s16(sum_s16x8, 1)
                            +vgetq_lane_s16(sum_s16x8, 2)
                            +vgetq_lane_s16(sum_s16x8, 3)
                            +vgetq_lane_s16(sum_s16x8, 4)
                            +vgetq_lane_s16(sum_s16x8, 5)
                            +vgetq_lane_s16(sum_s16x8, 6)
                            +vgetq_lane_s16(sum_s16x8, 7)
                            );
                }
                for (; i < (vx_int32)(conv_width * conv_height); ++i)
                    sum += conv_mat[i] * slice[i];
            }
            else if (src_format == VX_DF_IMAGE_S16)
            {
                vx_int16 slice[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };
                auxReadRect(src_base, &src_addr, bordermode, src_format, x, y, conv_radius_x, conv_radius_y, slice);
                
                if (y < CONV_DIM_HALF)
                {
                    INSERT_ZERO_Y(slice, y)
                }
                else if (y >= high_y - CONV_DIM_HALF)
                {
                    INSERT_VALUES_Y(slice, y)
                }

                if (x < CONV_DIM_HALF)
                {
                    INSERT_ZERO_X(slice, x)
                }
                else if (x >= high_x - CONV_DIM_HALF)
                {
                    INSERT_VALUES_X(slice, x)
                }

                i = 0;
                for(; i < roiw8; i+=8)
                {
                    int16x8_t conv_s16x8 = vld1q_s16(&conv_mat[i]);
                    int16x8_t slice_s16x8 = vld1q_s16(&slice[i]);
                    int16x8_t sum_s16x8 = vmulq_s16(conv_s16x8, slice_s16x8);
                    sum += (vgetq_lane_s16(sum_s16x8, 0)
                            +vgetq_lane_s16(sum_s16x8, 1)
                            +vgetq_lane_s16(sum_s16x8, 2)
                            +vgetq_lane_s16(sum_s16x8, 3)
                            +vgetq_lane_s16(sum_s16x8, 4)
                            +vgetq_lane_s16(sum_s16x8, 5)
                            +vgetq_lane_s16(sum_s16x8, 6)
                            +vgetq_lane_s16(sum_s16x8, 7)
                            );
                }
                for (; i < (vx_int32)(conv_width * conv_height); ++i)
                    sum += conv_mat[i] * slice[i];
            }

            value = sum / (vx_int32)scale;

            if (dst_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (value < 0) *dstp = 0;
                else if (value > UINT8_MAX) *dstp = UINT8_MAX;
                else *dstp = value;
            }
            else if (dst_format == VX_DF_IMAGE_S16)
            {
                vx_int16 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (value < INT16_MIN) *dstp = INT16_MIN;
                else if (value > INT16_MAX) *dstp = INT16_MAX;
                else *dstp = value;
            }
        }
    }

    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

    return status;
}


/* Laplacian Reconstruct */
static vx_status upsampleImage(vx_context context, vx_uint32 width, vx_uint32 height, vx_image filling, vx_convolution conv, vx_image upsample, vx_border_t *border)
{
    vx_status status = VX_SUCCESS;
    vx_df_image format, filling_format;

    format = VX_DF_IMAGE_U8;
    vx_image tmp = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
    status |= vxQueryImage(filling, VX_IMAGE_FORMAT, &filling_format, sizeof(filling_format));

    vx_rectangle_t tmp_rect, filling_rect;
    vx_imagepatch_addressing_t tmp_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t filling_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id tmp_map_id, filling_map_id;
    void *tmp_base = NULL;
    void *filling_base = NULL;

    status = vxGetValidRegionImage(tmp, &tmp_rect);
    status |= vxMapImagePatch(tmp, &tmp_rect, 0, &tmp_map_id, &tmp_addr, (void **)&tmp_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status = vxGetValidRegionImage(filling, &filling_rect);
    status |= vxMapImagePatch(filling, &filling_rect, 0, &filling_map_id, &filling_addr, (void **)&filling_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);

    for (int ix = 0; ix < width; ix++)
    {
        for (int iy = 0; iy < height; iy++)
        {

            void* tmp_datap = vxFormatImagePatchAddress2d(tmp_base, ix, iy, &tmp_addr);

            if (iy % 2 != 0 || ix % 2 != 0)
            {
                if (format == VX_DF_IMAGE_U8)
                    *(vx_uint8 *)tmp_datap = (vx_uint8)0;
                else
                    *(vx_int16 *)tmp_datap = (vx_int16)0;
            }
            else
            {
                void* filling_tmp = vxFormatImagePatchAddress2d(filling_base, ix / 2, iy / 2, &filling_addr);
                vx_int32 filling_data = filling_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)filling_tmp : *(vx_int16 *)filling_tmp;
                if (format == VX_DF_IMAGE_U8)
                {
                    if (filling_data > UINT8_MAX)
                        filling_data = UINT8_MAX;
                    else if (filling_data < 0)
                        filling_data = 0;
                    *(vx_uint8 *)tmp_datap = (vx_uint8)filling_data;
                }
                else
                {
                    if (filling_data > INT16_MAX)
                        filling_data = INT16_MAX;
                    else if (filling_data < INT16_MIN)
                        filling_data = INT16_MIN;
                    *(vx_int16 *)tmp_datap = (vx_int16)filling_data;
                }
            }
        }
    }

    status |= vxUnmapImagePatch(tmp, tmp_map_id);
    status |= vxUnmapImagePatch(filling, filling_map_id);

    status |= replicateConvolve(tmp, conv, upsample, border);

    vx_rectangle_t upsample_rect;
    vx_imagepatch_addressing_t upsample_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id upsample_map_id;
    void * upsample_base = NULL;
    vx_df_image upsample_format;

    status |= vxQueryImage(upsample, VX_IMAGE_FORMAT, &upsample_format, sizeof(upsample_format));
    status = vxGetValidRegionImage(upsample, &upsample_rect);
    status |= vxMapImagePatch(upsample, &upsample_rect, 0, &upsample_map_id, &upsample_addr, (void **)&upsample_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);

    for (int ix = 0; ix < width; ix++)
    {
        for (int iy = 0; iy < height; iy++)
        {
            void* upsample_p = vxFormatImagePatchAddress2d(upsample_base, ix, iy, &upsample_addr);
            vx_int32 upsample_data = upsample_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)upsample_p : *(vx_int16 *)upsample_p;
            upsample_data *= 4;
            if (upsample_format == VX_DF_IMAGE_U8)
            {
                if (upsample_data > UINT8_MAX)
                    upsample_data = UINT8_MAX;
                else if (upsample_data < 0)
                    upsample_data = 0;
                *(vx_uint8 *)upsample_p = (vx_uint8)upsample_data;
            }
            else
            {
                if (upsample_data > INT16_MAX)
                    upsample_data = INT16_MAX;
                else if (upsample_data < INT16_MIN)
                    upsample_data = INT16_MIN;
                *(vx_int16 *)upsample_p = (vx_int16)upsample_data;
            }
        }
    }
    status |= vxUnmapImagePatch(upsample, upsample_map_id);
    status |= vxReleaseImage(&tmp);
    return status;
}

#define VX_SCALE_PYRAMID_DOUBLE (2.0f)

static vx_param_description_t laplacian_reconstruct_kernel_params[] =
{
    { VX_INPUT, VX_TYPE_PYRAMID, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static vx_status VX_CALLBACK vxLaplacianReconstructKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == dimof(laplacian_reconstruct_kernel_params))
    {
        status = VX_SUCCESS;
    }
    return status;
}

static vx_status VX_CALLBACK vxLaplacianReconstructInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_SUCCESS;
    if (index == 0)
    {
        vx_pyramid laplacian = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        status |= vxQueryParameter(param, VX_PARAMETER_REF, &laplacian, sizeof(laplacian));
        if (status == VX_SUCCESS && laplacian)
        {
            vx_df_image pyr_format = 0;
            vx_float32  pyr_scale = 0;

            status |= vxQueryPyramid(laplacian, VX_PYRAMID_FORMAT, &pyr_format, sizeof(pyr_format));
            status |= vxQueryPyramid(laplacian, VX_PYRAMID_SCALE, &pyr_scale, sizeof(pyr_scale));

            if (status == VX_SUCCESS)
            {
                if ((pyr_format != VX_DF_IMAGE_S16) || (pyr_scale != VX_SCALE_PYRAMID_HALF))
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            status |= vxReleasePyramid(&laplacian);
        }

        status |= vxReleaseParameter(&param);
    }

    if (index == 1)
    {
        vx_pyramid laplacian = 0;
        vx_image input = 0;

        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        vx_parameter param1 = vxGetParameterByIndex(node, index);

        status |= vxQueryParameter(param0, VX_PARAMETER_REF, &laplacian, sizeof(laplacian));
        status |= vxQueryParameter(param1, VX_PARAMETER_REF, &input, sizeof(input));
        if (status == VX_SUCCESS && laplacian && input)
        {
            vx_size     pyr_levels = 0;
            vx_uint32   pyr_last_level_width = 0;
            vx_uint32   pyr_last_level_height = 0;
            vx_image    pyr_last_level_img = 0;
            vx_uint32   lowest_res_width = 0;
            vx_uint32   lowest_res_height = 0;
            vx_df_image lowest_res_format = 0;

            status |= vxQueryPyramid(laplacian, VX_PYRAMID_LEVELS, &pyr_levels, sizeof(pyr_levels));

            pyr_last_level_img = vxGetPyramidLevel(laplacian, (vx_uint32)(pyr_levels - 1));

            status |= vxQueryImage(pyr_last_level_img, VX_IMAGE_WIDTH, &pyr_last_level_width, sizeof(pyr_last_level_width));
            status |= vxQueryImage(pyr_last_level_img, VX_IMAGE_HEIGHT, &pyr_last_level_height, sizeof(pyr_last_level_height));

            status |= vxQueryImage(input, VX_IMAGE_WIDTH, &lowest_res_width, sizeof(lowest_res_width));
            status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &lowest_res_height, sizeof(lowest_res_height));
            status |= vxQueryImage(input, VX_IMAGE_FORMAT, &lowest_res_format, sizeof(lowest_res_format));

            if (status == VX_SUCCESS)
            {
                if ((pyr_last_level_width != lowest_res_width * 2) || (pyr_last_level_height != lowest_res_height * 2))
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
            }

            status |= vxReleasePyramid(&laplacian);
            status |= vxReleaseImage(&input);
            status |= vxReleaseImage(&pyr_last_level_img);
        }

        status |= vxReleaseParameter(&param0);
        status |= vxReleaseParameter(&param1);
    }

    return status;
}

static vx_status VX_CALLBACK vxLaplacianReconstructOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t* ptr)
{
    vx_status status = VX_SUCCESS;
    if (index == 2)
    {
        vx_pyramid laplacian = 0;
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);

        status |= vxQueryParameter(src_param, VX_PARAMETER_REF, &laplacian, sizeof(laplacian));
        if (status == VX_SUCCESS && laplacian)
        {
            vx_uint32 pyr_width = 0;
            vx_uint32 pyr_height = 0;
            vx_image output = 0;

            status |= vxQueryPyramid(laplacian, VX_PYRAMID_WIDTH, &pyr_width, sizeof(pyr_width));
            status |= vxQueryPyramid(laplacian, VX_PYRAMID_HEIGHT, &pyr_height, sizeof(pyr_height));

            status |= vxQueryParameter(dst_param, VX_PARAMETER_REF, &output, sizeof(output));

            if (status == VX_SUCCESS && output)
            {
                vx_uint32 dst_width = 0;
                vx_uint32 dst_height = 0;
                vx_df_image dst_format = 0;

                status |= vxQueryImage(output, VX_IMAGE_WIDTH, &dst_width, sizeof(dst_width));
                status |= vxQueryImage(output, VX_IMAGE_HEIGHT, &dst_height, sizeof(dst_height));
                status |= vxQueryImage(output, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

                if (status == VX_SUCCESS)
                {
                    if (dst_width == pyr_width && dst_height == pyr_height)
                    {
                        /* fill in the meta data with the attributes so that the checker will pass */
                        ptr->type = VX_TYPE_IMAGE;
                        ptr->dim.image.width = dst_width;
                        ptr->dim.image.height = dst_height;
                        ptr->dim.image.format = dst_format;
                    }
                    else
                        status = VX_ERROR_INVALID_PARAMETERS;
                }

                status |= vxReleaseImage(&output);
            }

            status |= vxReleasePyramid(&laplacian);
        }

        status |= vxReleaseParameter(&dst_param);
        status |= vxReleaseParameter(&src_param);
    }

    return status;
}

static vx_status ownCopyImage_S16(vx_image input, vx_image output)
{
    vx_status status = VX_SUCCESS; // assume success until an error occurs.
    vx_uint32 p = 0;
    vx_uint32 y = 0, x = 0;
    vx_size planes = 0;

    void* src;
    void* dst;
    vx_imagepatch_addressing_t src_addr;
    vx_imagepatch_addressing_t dst_addr;
    vx_rectangle_t src_rect, dst_rect;
    vx_map_id map_id1;
    vx_map_id map_id2;
    vx_df_image src_format = 0;
    vx_df_image out_format = 0;

    status |= vxQueryImage(input, VX_IMAGE_PLANES, &planes, sizeof(planes));
    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(input, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    
    status |= vxGetValidRegionImage(input, &src_rect);
    status |= vxGetValidRegionImage(output, &dst_rect);

    vx_uint32 wWidth;
    for (p = 0; p < planes && status == VX_SUCCESS; p++)
    {
        status = VX_SUCCESS;

        src = NULL;
        dst = NULL;

        status |= vxMapImagePatch(input, &src_rect, p, &map_id1, &src_addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        status |= vxMapImagePatch(output, &dst_rect, p, &map_id2, &dst_addr, &dst, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

        wWidth = (src_addr.dim_x >> 3) << 3;
        for (y = 0; y < src_addr.dim_y && status == VX_SUCCESS; y += src_addr.step_y)
        {
            vx_int16 *ptr_dst = (vx_int16 *)dst + y * dst_addr.stride_y/2;
            vx_uint8 *ptr_src = (vx_uint8 *)src + y * src_addr.stride_y;
            for (x = 0; x < wWidth; x += 8)
            {
                if (src_format == VX_DF_IMAGE_U8)
                {
                    uint8x8_t vSrc = vld1_u8(ptr_src + x * src_addr.stride_x);
                    uint16x8_t vSrcu16 = vmovl_u8(vSrc);
                    int16x8_t vSrcs16 = vcombine_s16(vreinterpret_s16_u16(vget_low_u16(vSrcu16)),
                        vreinterpret_s16_u16(vget_high_u16(vSrcu16)));
                    vst1q_s16(ptr_dst + x, vSrcs16);
                }
                else if (src_format == VX_DF_IMAGE_S16)
                {
                    int16x8_t vSrc = vld1q_s16((vx_int16 *)(ptr_src + x * src_addr.stride_x));
                    vst1q_s16(ptr_dst + x, vSrc);
                }
            }
            for (x = wWidth; x < src_addr.dim_x; x += src_addr.step_x)
            {
                void* srcp = vxFormatImagePatchAddress2d(src, x, y, &src_addr);
                vx_int32 out0 = src_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)srcp : *(vx_int16 *)srcp;
                ptr_dst[x] = out0;
            }
        }

        if (status == VX_SUCCESS)
        {
            status |= vxUnmapImagePatch(input, map_id1);
            status |= vxUnmapImagePatch(output, map_id2);
        }
    }

    return status;
}

static vx_status VX_CALLBACK vxLaplacianReconstructInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_SUCCESS;

    if (num == dimof(laplacian_reconstruct_kernel_params))
    {
        vx_context context = vxGetContext((vx_reference)node);

        vx_size lev;
        vx_size levels = 1;
        vx_uint32 width = 0;
        vx_uint32 height = 0;
        vx_uint32 level_width = 0;
        vx_uint32 level_height = 0;
        vx_df_image format = VX_DF_IMAGE_S16;
        vx_enum policy = VX_CONVERT_POLICY_SATURATE;
        vx_border_t border;
        vx_image filling = 0;
        vx_image pyr_level = 0;
        vx_image filter = 0;
        vx_image out = 0;
        vx_convolution conv;

        vx_pyramid laplacian = (vx_pyramid)parameters[0];
        vx_image   input = (vx_image)parameters[1];
        vx_image   output = (vx_image)parameters[2];

        vx_scalar spolicy = vxCreateScalar(context, VX_TYPE_ENUM, &policy);

        status |= vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
        status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));

        status |= vxQueryPyramid(laplacian, VX_PYRAMID_LEVELS, &levels, sizeof(levels));

        status |= vxQueryNode(node, VX_NODE_BORDER, &border, sizeof(border));
        border.mode = VX_BORDER_REPLICATE;
        conv = vxCreateGaussian5x5Convolution(context);

        level_width = (vx_uint32)ceilf(width  * VX_SCALE_PYRAMID_DOUBLE);
        level_height = (vx_uint32)ceilf(height * VX_SCALE_PYRAMID_DOUBLE);
        filling = vxCreateImage(context, width, height, format);
        for (lev = 0; lev < levels; lev++)
        {
            out = vxCreateImage(context, level_width, level_height, format);
            filter = vxCreateImage(context, level_width, level_height, format);

            pyr_level = vxGetPyramidLevel(laplacian, (vx_uint32)((levels - 1) - lev));

            if (lev == 0)
            {
                ownCopyImage_S16(input, filling);
            }
            upsampleImage(context, level_width, level_height, filling, conv, filter, &border);
            vxAddition(filter, pyr_level, spolicy, out);

            status |= vxReleaseImage(&pyr_level);

            if ((levels - 1) - lev == 0)
            {
                ownCopyImage(out, output);
                status |= vxReleaseImage(&filling);
            }
            else
            {
                /* compute dimensions for the next level */
                status |= vxReleaseImage(&filling);
                filling = vxCreateImage(context, level_width, level_height, format);
                ownCopyImage(out, filling);

                level_width = (vx_uint32)ceilf(level_width  * VX_SCALE_PYRAMID_DOUBLE);
                level_height = (vx_uint32)ceilf(level_height * VX_SCALE_PYRAMID_DOUBLE);


            }
            status |= vxReleaseImage(&out);
            status |= vxReleaseImage(&filter);

        }
        status |= vxReleaseConvolution(&conv);
        status |= vxReleaseScalar(&spolicy);
    }

    return status;
}

static vx_status VX_CALLBACK vxLaplacianReconstructDeinitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == dimof(laplacian_reconstruct_kernel_params))
    {
        vx_graph subgraph = ownGetChildGraphOfNode(node);
        vx_context context = vxGetContext((vx_reference)node);
        status = VX_SUCCESS;
        status |= vxReleaseGraph(&subgraph);
        /* set subgraph to "null" */
        status |= ownSetChildGraphOfNode(node, 0);
        status |= vxUnloadKernels(context, "openvx-debug");
    }
    return status;
}

vx_kernel_description_t laplacian_reconstruct_kernel =
{
    VX_KERNEL_LAPLACIAN_RECONSTRUCT,
    "org.khronos.openvx.laplacian_reconstruct",
    vxLaplacianReconstructKernel,
    laplacian_reconstruct_kernel_params, dimof(laplacian_reconstruct_kernel_params),
    NULL,
    vxLaplacianReconstructInputValidator,
    vxLaplacianReconstructOutputValidator,
    vxLaplacianReconstructInitializer,
    vxLaplacianReconstructDeinitializer,
};
