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
#include <arm_neon.h>
#include <VX/vxu.h>
#include "vx_internal.h"

static const vx_uint32 gaussian5x5scale = 256;
static const vx_int16 gaussian5x5[5][5] =
{
    {1,  4,  6,  4, 1},
    {4, 16, 24, 16, 4},
    {6, 24, 36, 24, 6},
    {4, 16, 24, 16, 4},
    {1,  4,  6,  4, 1}
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

static vx_status xLaplacianPyramidupsampleImage(vx_context context, vx_uint32 width, vx_uint32 height, vx_image filling, vx_convolution conv, vx_image upsample, vx_border_t *border)
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
    
    for (int iy = 0; iy < height; iy++)
    {
        vx_uint8 *tmp_datap =  (vx_uint8 *)tmp_base + iy * width;
        vx_uint8  *ptr_src = (vx_uint8 *)filling_base + (iy >> 1) * (width >> 1);

        if (iy % 2!=0)
        {
            memset(tmp_datap,0,sizeof(vx_uint8)*width);
        }
        else
        {
            for (int ix = 0; ix < width; ix++)
            {
                if (ix % 2 != 0 || iy % 2 !=0)
                {
                    *(vx_uint8 *)tmp_datap = (vx_uint8)0;
                }
                else
                {
                    *(vx_uint8 *)tmp_datap = ptr_src[ix/2];
                }
                tmp_datap++; 
            }
        }
    }
    status |= vxUnmapImagePatch(tmp, tmp_map_id);
    status |= vxUnmapImagePatch(filling, filling_map_id);
    
    status |= vxGaussian5x5( tmp, upsample, border );
    
    vx_rectangle_t upsample_rect;
    vx_imagepatch_addressing_t upsample_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id upsample_map_id;
    void * upsample_base = NULL;
    vx_df_image upsample_format;

    status |= vxQueryImage(upsample, VX_IMAGE_FORMAT, &upsample_format, sizeof(upsample_format));
    status = vxGetValidRegionImage(upsample, &upsample_rect);
    status |= vxMapImagePatch(upsample, &upsample_rect, 0, &upsample_map_id, &upsample_addr, (void **)&upsample_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);

     vx_uint32 w8 = width >= 7 ? width - 7 : 0;
     int16x8_t sh = vdupq_n_s16(2);

     for (int iy = 0; iy < height; iy++)
     {
        vx_int16* upsample_p = (vx_int16 *)upsample_base + iy * width;
        int ix=0;

        for (ix =0; ix < w8; ix+=8, upsample_p+=8)
        {
            int16x8_t upsample_v = vld1q_s16( upsample_p );
            int16x8_t upsample_data = vqshlq_s16(upsample_v,sh);
            vst1q_s16( upsample_p , upsample_data );
        }
        for (; ix < width; ix++, upsample_p++)
        {
            vx_int32 upsample_data = (*(vx_int16 *)upsample_p)*4;
            if (upsample_data > INT16_MAX)
                upsample_data = INT16_MAX;
            else if (upsample_data < INT16_MIN)
                 upsample_data = INT16_MIN;
            *(vx_int16 *)upsample_p = (vx_int16)upsample_data;
        }
    }
    status |= vxUnmapImagePatch(upsample, upsample_map_id);
    status |= vxReleaseImage(&tmp);

    return status;
}



/* --------------------------------------------------------------------------*/
/* Laplacian Pyramid */

static vx_param_description_t laplacian_pyramid_kernel_params[] =
{
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_PYRAMID, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};


static vx_status VX_CALLBACK vxLaplacianPyramidKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)

{

    vx_status status = VX_FAILURE;
    if (num == dimof(laplacian_pyramid_kernel_params))
    {
        status = VX_SUCCESS;
    }
    return status;

}

static vx_status VX_CALLBACK vxLaplacianPyramidInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_SUCCESS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        status |= vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (status == VX_SUCCESS && input)
        {
            vx_df_image format = 0;
            status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (status == VX_SUCCESS)
            {
                if (format != VX_DF_IMAGE_U8)
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            status |= vxReleaseImage(&input);
        }

        status |= vxReleaseParameter(&param);
    }

    return status;
}


static vx_status VX_CALLBACK vxLaplacianPyramidOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t* ptr)
{
    vx_status status = VX_SUCCESS;
    if (index == 1)
    {
        vx_image input = 0;
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);

        status |= vxQueryParameter(src_param, VX_PARAMETER_REF, &input, sizeof(input));
        if (status == VX_SUCCESS && input)
        {
            vx_uint32 width = 0;
            vx_uint32 height = 0;
            vx_df_image format = 0;
            vx_pyramid laplacian = 0;

            status |= vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
            status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
            status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

            status |= vxQueryParameter(dst_param, VX_PARAMETER_REF, &laplacian, sizeof(laplacian));
            if (status == VX_SUCCESS && laplacian)
            {
                vx_uint32 pyr_width = 0;
                vx_uint32 pyr_height = 0;
                vx_df_image pyr_format = 0;
                vx_float32 pyr_scale = 0;
                vx_size pyr_levels = 0;

                status |= vxQueryPyramid(laplacian, VX_PYRAMID_WIDTH, &pyr_width, sizeof(pyr_width));
                status |= vxQueryPyramid(laplacian, VX_PYRAMID_HEIGHT, &pyr_height, sizeof(pyr_height));
                status |= vxQueryPyramid(laplacian, VX_PYRAMID_FORMAT, &pyr_format, sizeof(pyr_format));
                status |= vxQueryPyramid(laplacian, VX_PYRAMID_SCALE, &pyr_scale, sizeof(pyr_scale));
                status |= vxQueryPyramid(laplacian, VX_PYRAMID_LEVELS, &pyr_levels, sizeof(pyr_levels));

                if (status == VX_SUCCESS)
                {
                    if (pyr_width == width && pyr_height == height && pyr_format == VX_DF_IMAGE_S16 && pyr_scale == VX_SCALE_PYRAMID_HALF)
                    {
                        /* fill in the meta data with the attributes so that the checker will pass */
                        ptr->type = VX_TYPE_PYRAMID;
                        ptr->dim.pyramid.width = pyr_width;
                        ptr->dim.pyramid.height = pyr_height;
                        ptr->dim.pyramid.format = pyr_format;
                        ptr->dim.pyramid.scale = pyr_scale;
                        ptr->dim.pyramid.levels = pyr_levels;
                    }
                    else
                        status = VX_ERROR_INVALID_PARAMETERS;
                }

                status |= vxReleasePyramid(&laplacian);
            }

            status |= vxReleaseImage(&input);
        }

        status |= vxReleaseParameter(&dst_param);
        status |= vxReleaseParameter(&src_param);
    }

    if (index == 2)
    {
        vx_pyramid laplacian = 0;
        vx_parameter src_param = vxGetParameterByIndex(node, 1);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);

        status |= vxQueryParameter(src_param, VX_PARAMETER_REF, &laplacian, sizeof(laplacian));
        if (status == VX_SUCCESS && laplacian)
        {
            vx_uint32 width = 0;
            vx_uint32 height = 0;
            vx_df_image format = 0;
            vx_size levels = 0;
            vx_image last_level = 0;
            vx_image output = 0;

            status |= vxQueryPyramid(laplacian, VX_PYRAMID_LEVELS, &levels, sizeof(levels));

            last_level = vxGetPyramidLevel(laplacian, (vx_uint32)(levels - 1));

            status |= vxQueryImage(last_level, VX_IMAGE_WIDTH, &width, sizeof(width));
            status |= vxQueryImage(last_level, VX_IMAGE_HEIGHT, &height, sizeof(height));
            status |= vxQueryImage(last_level, VX_IMAGE_FORMAT, &format, sizeof(format));

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
                    if (dst_width == width / 2 && dst_height == height / 2)
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

            status |= vxReleaseImage(&last_level);
            status |= vxReleasePyramid(&laplacian);
        }

        status |= vxReleaseParameter(&dst_param);
        status |= vxReleaseParameter(&src_param);
    }

    return status;
}
static vx_status VX_CALLBACK vxLaplacianPyramidInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_SUCCESS;

    if (num == dimof(laplacian_pyramid_kernel_params))
    {
        vx_context context = vxGetContext((vx_reference)node);

        vx_size lev;
        vx_size levels = 1;
        vx_uint32 width = 0;
        vx_uint32 height = 0;
        vx_uint32 level_width = 0;
        vx_uint32 level_height = 0;
        vx_df_image format;
        vx_enum policy = VX_CONVERT_POLICY_SATURATE;
        vx_border_t border;
        vx_convolution conv = 0;
        vx_image pyr_gauss_curr_level_filtered = 0;
        vx_image pyr_laplacian_curr_level = 0;
        vx_image   input = (vx_image)parameters[0];
        vx_pyramid laplacian = (vx_pyramid)parameters[1];
        vx_image   output = (vx_image)parameters[2];
        vx_pyramid gaussian = 0;
        vx_image gauss_cur = 0;
        vx_image gauss_next = 0;
        vx_image upsample = 0;

        status |= vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
        status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
        status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

        status |= vxQueryPyramid(laplacian, VX_PYRAMID_LEVELS, &levels, sizeof(levels));

        status |= vxQueryNode(node, VX_NODE_BORDER, &border, sizeof(border));
        
        border.mode = VX_BORDER_REPLICATE;

        gaussian = vxCreatePyramid(context, levels + 1, VX_SCALE_PYRAMID_HALF, width, height, VX_DF_IMAGE_U8);
        vxuGaussianPyramid(context, input, gaussian);

        conv = vxCreateGaussian5x5Convolution(context);

        level_width = width;
        level_height = height;

        gauss_cur = vxGetPyramidLevel(gaussian, 0);
        gauss_next = vxGetPyramidLevel(gaussian, 1);
        for (lev = 0; lev < levels; lev++)
        {
            pyr_gauss_curr_level_filtered = vxCreateImage(context, level_width, level_height, VX_DF_IMAGE_S16);
            xLaplacianPyramidupsampleImage(context, level_width, level_height, gauss_next, conv, pyr_gauss_curr_level_filtered, &border);

            pyr_laplacian_curr_level = vxGetPyramidLevel(laplacian, (vx_uint32)lev);
            status |= vxuSubtract(context, gauss_cur, pyr_gauss_curr_level_filtered, policy, pyr_laplacian_curr_level);
            vxReleaseImage(&upsample);

            if (lev == levels - 1)
            {
                vx_image tmp = vxGetPyramidLevel(gaussian, levels);
                ownCopyImage(tmp, output);
                vxReleaseImage(&tmp);
                vxReleaseImage(&gauss_next);
                vxReleaseImage(&gauss_cur);
            }
            else
            {
                /* compute dimensions for the next level */
                level_width = (vx_uint32)ceilf(level_width * VX_SCALE_PYRAMID_HALF);
                level_height = (vx_uint32)ceilf(level_height * VX_SCALE_PYRAMID_HALF);
                /* prepare to the next iteration */
                /* make the next level of gaussian pyramid the current level */
                vxReleaseImage(&gauss_next);
                vxReleaseImage(&gauss_cur);
                gauss_cur = vxGetPyramidLevel(gaussian, lev + 1);
                gauss_next = vxGetPyramidLevel(gaussian, lev + 2);

            }

            /* decrements the references */

            status |= vxReleaseImage(&pyr_gauss_curr_level_filtered);
            status |= vxReleaseImage(&pyr_laplacian_curr_level);
        }

        status |= vxReleasePyramid(&gaussian);
        status |= vxReleaseConvolution(&conv);
    }

    return status;
}

static vx_status VX_CALLBACK vxLaplacianPyramidDeinitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == dimof(laplacian_pyramid_kernel_params))
    {
        status = VX_SUCCESS;
    }
    return status;
}

vx_kernel_description_t laplacian_pyramid_kernel =
{
    VX_KERNEL_LAPLACIAN_PYRAMID,
    "org.khronos.openvx.laplacian_pyramid",
    vxLaplacianPyramidKernel,
    laplacian_pyramid_kernel_params, dimof(laplacian_pyramid_kernel_params),
    NULL,
    vxLaplacianPyramidInputValidator,
    vxLaplacianPyramidOutputValidator,
    vxLaplacianPyramidInitializer,
    vxLaplacianPyramidDeinitializer,
};









