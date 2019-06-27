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

    uint32_t wWidth;
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
            uint8_t *ptr_src = (uint8_t *)src + y * src_addr.stride_y;
            uint8_t *ptr_dst = (uint8_t *)dst + y * dst_addr.stride_y;
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

    uint32_t mid_width = 0;
    uint32_t mid_height = 0;

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
