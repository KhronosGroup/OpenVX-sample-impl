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

#include <c_model.h>

#include <VX/vx_types.h>
#include <VX/vx_lib_extras.h>
#include <stdio.h>


// nodeless version of the ConvertDepth kernel
vx_status vxConvertDepth(vx_image input, vx_image output, vx_scalar spol, vx_scalar sshf)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_enum format[2];
    vx_enum policy = 0;
    vx_int32 shift = 0;

    vx_status status = VX_SUCCESS;
    status |= vxCopyScalar(spol, &policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(sshf, &shift, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format[0], sizeof(format[0]));
    status |= vxQueryImage(output, VX_IMAGE_FORMAT, &format[1], sizeof(format[1]));
    status |= vxGetValidRegionImage(input, &rect);
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    width  = (format[0] == VX_DF_IMAGE_U1) ? src_addr.dim_x - rect.start_x % 8 : src_addr.dim_x;
    height = src_addr.dim_y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            if ((format[0] == VX_DF_IMAGE_U1) && (format[1] == VX_DF_IMAGE_U8))
            {
                vx_uint32 xSrc = x + rect.start_x % 8;   // U1 valid region offset
                vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, xSrc, y, &src_addr);
                vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x,    y, &dst_addr);
                *dst = ((*src & (1 << (xSrc % 8))) != 0 ? 255u : 0u);
            }
            if ((format[0] == VX_DF_IMAGE_U1) && (format[1] == VX_DF_IMAGE_U16))
            {
                vx_uint32 xSrc = x + rect.start_x % 8;   // U1 valid region offset
                vx_uint8  *src = (vx_uint8*) vxFormatImagePatchAddress2d(src_base, xSrc, y, &src_addr);
                vx_uint16 *dst = (vx_uint16*)vxFormatImagePatchAddress2d(dst_base, x,    y, &dst_addr);
                *dst = ((vx_uint16)(*src & (1 << (xSrc % 8))) != 0 ? 65535u : 0u);
            }
            if ((format[0] == VX_DF_IMAGE_U1) && (format[1] == VX_DF_IMAGE_S16))
            {
                vx_uint32 xSrc = x + rect.start_x % 8;   // U1 valid region offset
                vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, xSrc, y, &src_addr);
                vx_int16  *dst = (vx_int16*)vxFormatImagePatchAddress2d(dst_base, x,    y, &dst_addr);
                *dst = ((vx_int16)(*src & (1 << (xSrc % 8))) != 0 ? -1 : 0);
            }
            if ((format[0] == VX_DF_IMAGE_U1) && (format[1] == VX_DF_IMAGE_U32))
            {
                vx_uint32 xSrc = x + rect.start_x % 8;   // U1 valid region offset
                vx_uint8  *src = (vx_uint8*) vxFormatImagePatchAddress2d(src_base, xSrc, y, &src_addr);
                vx_uint32 *dst = (vx_uint32*)vxFormatImagePatchAddress2d(dst_base, x,    y, &dst_addr);
                *dst = ((vx_uint32)(*src & (1 << (xSrc % 8))) != 0 ? 4294967295u : 0u);
            }
            if ((format[0] == VX_DF_IMAGE_U8) && (format[1] == VX_DF_IMAGE_U1))
            {
                vx_uint32 xDst = x + rect.start_x % 8;   // U1 valid region offset
                vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,    y, &src_addr);
                vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xDst, y, &dst_addr);
                *dst = (*dst & ~(1 << (xDst % 8))) | ((*src != 0 ? 1u : 0u) << (xDst % 8));
            }
            if ((format[0] == VX_DF_IMAGE_U8) && (format[1] == VX_DF_IMAGE_U16))
            {
                vx_uint8  *src = (vx_uint8*) vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint16 *dst = (vx_uint16*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = ((vx_uint16)(*src)) << shift;
            }
            else if ((format[0] == VX_DF_IMAGE_U8) && (format[1] == VX_DF_IMAGE_S16))
            {
                vx_uint8 *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_int16 *dst = (vx_int16*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = ((vx_int16)(*src)) << shift;
            }
            else if ((format[0] == VX_DF_IMAGE_U8) && (format[1] == VX_DF_IMAGE_U32))
            {
                vx_uint8  *src = (vx_uint8*) vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint32 *dst = (vx_uint32*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = ((vx_uint32)(*src)) << shift;
            }
            else if ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U1))
            {
                vx_uint32 xDst = x + rect.start_x % 8;   // U1 valid region offset
                vx_uint16 *src = (vx_uint16*)vxFormatImagePatchAddress2d(src_base, x,    y, &src_addr);
                vx_uint8  *dst = (vx_uint8*) vxFormatImagePatchAddress2d(dst_base, xDst, y, &dst_addr);
                *dst = (*dst & ~(1 << (xDst % 8))) | ((vx_uint8)(*src != 0 ? 1u : 0u) << (xDst % 8));
            }
            else if ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U32))
            {
                vx_uint16 *src = (vx_uint16*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint32 *dst = (vx_uint32*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = ((vx_uint32)(*src)) << shift;
            }
            else if ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_U1))
            {
                vx_uint32 xDst = x + rect.start_x % 8;   // U1 valid region offset
                vx_int16  *src = (vx_int16*)vxFormatImagePatchAddress2d(src_base, x,    y, &src_addr);
                vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xDst, y, &dst_addr);
                *dst = (*dst & ~(1 << (xDst % 8))) | ((vx_uint8)(*src != 0 ? 1u : 0u) << (xDst % 8));
            }
            else if ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_S32))
            {
                vx_int16 *src = (vx_int16*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_int32 *dst = (vx_int32*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = ((vx_int32)(*src)) << shift;
            }
            else if ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U8))
            {
                vx_uint16 *src = (vx_uint16*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint8  *dst = (vx_uint8*) vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (policy == VX_CONVERT_POLICY_WRAP)
                {
                    *dst = (vx_uint8)((*src) >> shift);
                }
                else if (policy == VX_CONVERT_POLICY_SATURATE)
                {
                    vx_uint16 value = (*src) >> shift;
                    value = (value > UINT8_MAX ? UINT8_MAX : value);
                    *dst = (vx_uint8)value;
                }
            }
            else if ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_U8))
            {
                vx_int16 *src = (vx_int16*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (policy == VX_CONVERT_POLICY_WRAP)
                {
                    *dst = (vx_uint8)((*src) >> shift);
                }
                else if (policy == VX_CONVERT_POLICY_SATURATE)
                {
                    vx_int16 value = (*src) >> shift;
                    value = (value < 0 ? 0 : value);
                    value = (value > UINT8_MAX ? UINT8_MAX : value);
                    *dst = (vx_uint8)value;
                }
            }
            else if ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U1))
            {
                vx_uint32 xDst = x + rect.start_x % 8;   // U1 valid region offset
                vx_uint32 *src = (vx_uint32*)vxFormatImagePatchAddress2d(src_base, x,    y, &src_addr);
                vx_uint8  *dst = (vx_uint8*) vxFormatImagePatchAddress2d(dst_base, xDst, y, &dst_addr);
                *dst = (*dst & ~(1 << (xDst % 8))) | ((vx_uint8)(*src != 0 ? 1u : 0u) << (xDst % 8));
            }
            else if ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U8))
            {
                vx_uint32 *src = (vx_uint32*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint8  *dst = (vx_uint8*) vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (policy == VX_CONVERT_POLICY_WRAP)
                {
                    *dst = (vx_uint8)((*src) >> shift);
                }
                else if (policy == VX_CONVERT_POLICY_SATURATE)
                {
                    vx_uint32 value = (*src) >> shift;
                    value = (value > UINT8_MAX ? UINT8_MAX : value);
                    *dst = (vx_uint8)value;
                }
            }
            else if ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U16))
            {
                vx_uint32 *src = (vx_uint32*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint16 *dst = (vx_uint16*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (policy == VX_CONVERT_POLICY_WRAP)
                {
                    *dst = (vx_uint16)((*src) >> shift);
                }
                else if (policy == VX_CONVERT_POLICY_SATURATE)
                {
                    vx_uint32 value = (*src) >> shift;
                    value = (value > UINT16_MAX ? UINT16_MAX : value);
                    *dst = (vx_uint16)value;
                }
            }
            else if ((format[0] == VX_DF_IMAGE_S32) && (format[1] == VX_DF_IMAGE_S16))
            {
                vx_int32 *src = (vx_int32*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_int16 *dst = (vx_int16*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (policy == VX_CONVERT_POLICY_WRAP)
                {
                    *dst = (vx_int16)((*src) >> shift);
                }
                else if (policy == VX_CONVERT_POLICY_SATURATE)
                {
                    vx_int32 value = (*src) >> shift;
                    value = (value < INT16_MIN ? INT16_MIN : value);
                    value = (value > INT16_MAX ? INT16_MAX : value);
                    *dst = (vx_int16)value;
                }
            }
            else if ((format[0] == VX_DF_IMAGE_F32) && (format[1] == VX_DF_IMAGE_U8))
            {
                vx_float32 *src = (vx_float32*)vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint8   *dst = (vx_uint8*)  vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_float32 pf = floorf(log10f(*src));
                vx_int32 p32 = (vx_int32)pf;
                vx_uint8 p8 = (vx_uint8)(p32 > 255 ? 255 : (p32 < 0 ? 0 : p32));
                if (pf > 0.0f)
                {
//                    printf("Float power of %lf\n", pf); //removed this print. You get too much of those prints in harris corner
                }
                *dst = (vx_uint8)(p8 << shift);
            }
        }
    }
    status |= vxCommitImagePatch(input, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);

    return status;
}

