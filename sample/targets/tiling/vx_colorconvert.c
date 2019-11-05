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

#include "vx_interface.h"

#include "vx_internal.h"

#include "tiling.h"

static vx_status VX_CALLBACK vxColorConvertInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_SUCCESS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image image = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &image, sizeof(image));
            if (image)
            {
                vx_df_image format = 0;
                vx_uint32 width = 0, height = 0;

                vxQueryImage(image, VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(height));
                // check to make sure the input format is supported.
                switch (format)
                {
                    case VX_DF_IMAGE_RGB:  /* 8:8:8 interleaved */
                    case VX_DF_IMAGE_RGBX: /* 8:8:8:8 interleaved */
                    case VX_DF_IMAGE_NV12: /* 4:2:0 co-planar*/
                    case VX_DF_IMAGE_NV21: /* 4:2:0 co-planar*/
                    case VX_DF_IMAGE_IYUV: /* 4:2:0 planar */
                        if (height & 1)
                        {
                            status = VX_ERROR_INVALID_DIMENSION;
                            break;
                        }
                        /* no break */
                    case VX_DF_IMAGE_YUYV: /* 4:2:2 interleaved */
                    case VX_DF_IMAGE_UYVY: /* 4:2:2 interleaved */
                        if (width & 1)
                        {
                            status = VX_ERROR_INVALID_DIMENSION;
                        }
                        break;
                    default:
                        status = VX_ERROR_INVALID_FORMAT;
                        break;
                }
                vxReleaseImage(&image);
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            vxReleaseParameter(&param);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    return status;
}

static vx_df_image color_combos[][2] = {
        /* {src, dst} */
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_NV21},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_YUV4},
};

static vx_status VX_CALLBACK vxColorConvertOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        vx_parameter param1 = vxGetParameterByIndex(node, 1);
        if ((vxGetStatus((vx_reference)param0) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)param1) == VX_SUCCESS))
        {
            vx_image output = 0, input = 0;
            vxQueryParameter(param0, VX_PARAMETER_REF, &input, sizeof(input));
            vxQueryParameter(param1, VX_PARAMETER_REF, &output, sizeof(output));
            if (input && output)
            {
                vx_df_image src = VX_DF_IMAGE_VIRT;
                vx_df_image dst = VX_DF_IMAGE_VIRT;
                vxQueryImage(input, VX_IMAGE_FORMAT, &src, sizeof(src));
                vxQueryImage(output, VX_IMAGE_FORMAT, &dst, sizeof(dst));
                if (dst != VX_DF_IMAGE_VIRT) /* can't be a unspecified format */
                {
                    vx_uint32 i = 0;
                    for (i = 0; i < dimof(color_combos); i++)
                    {
                        if ((color_combos[i][0] == src) &&
                            (color_combos[i][1] == dst))
                        {
                            ptr->type = VX_TYPE_IMAGE;
                            ptr->dim.image.format = dst;
                            vxQueryImage(input, VX_IMAGE_WIDTH, &ptr->dim.image.width, sizeof(ptr->dim.image.width));
                            vxQueryImage(input, VX_IMAGE_HEIGHT, &ptr->dim.image.height, sizeof(ptr->dim.image.height));
                            status = VX_SUCCESS;
                            break;
                        }
                    }
                }
                vxReleaseImage(&input);
                vxReleaseImage(&output);
            }
            vxReleaseParameter(&param0);
            vxReleaseParameter(&param1);
        }
    }
    VX_PRINT(VX_ZONE_API, "%s:%u returned %d\n", __FUNCTION__, index, status);
    return status;
}

/*! \brief The exported kernel table entry */
vx_tiling_kernel_t colorconvert_kernel = 
{
    "org.khronos.openvx.tiling_color_convert",
    VX_KERNEL_COLOR_CONVERT_TILING,
    NULL,
    ConvertColor_image_tiling_flexible,
    ConvertColor_image_tiling_fast,
    2, 
    { { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED } },
    NULL,
    vxColorConvertInputValidator,
    vxColorConvertOutputValidator,
    NULL,
    NULL,
    { 8, 8 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};

