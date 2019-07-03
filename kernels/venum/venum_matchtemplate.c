/*

 * Copyright (c) 2017-2017 The Khronos Group Inc.
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
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <venum.h>
#include <arm_neon.h>

#define _SQRT_MAGIC     0xbe6f0000
/* calculates 1/sqrt(val) */
vx_float64
vxInvSqrt64d( vx_float64 arg )
{
    vx_float64 x, y;

    /*
        Warning: the following code makes assumptions
        about the format of float.
    */
    union {
        vx_float32 t;
        vx_uint32 u;
    } floatbits;

    floatbits.t = (vx_float32) arg;
    floatbits.u = (_SQRT_MAGIC - floatbits.u) >> 1;
    
    y = arg * 0.5;
    x = floatbits.t;
    x *= 1.5 - y * x * x;
    x *= 1.5 - y * x * x;
    x *= 1.5 - y * x * x;
    x *= 1.5 - y * x * x;
    return x;
}

vx_int64
vxCompareBlocksHamming_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, vx_int32 len )
{
    vx_int32 i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        vx_int32 v = vec1[i] ^ vec2[i];
        vx_int32 e = v;

        v = vec1[i + 1] ^ vec2[i + 1];
        e += v;
        v = vec1[i + 2] ^ vec2[i + 2];
        e += v;
        v = vec1[i + 3] ^ vec2[i + 3];
        e += v;
        sum += e;
    }
    for( ; i < len; i++ )
    {
        vx_int32 v = vec1[i] ^ vec2[i];
        s += v;
    }
    return sum + s;
}

/*
   Calculates cross correlation for two blocks.
*/

vx_int64
vxCrossCorr_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, vx_int32 len )
{
    vx_int32 i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        vx_int32 e = vec1[i] * vec2[i];
        vx_int32 v = vec1[i + 1] * vec2[i + 1];

        e += v;
        v = vec1[i + 2] * vec2[i + 2];
        e += v;
        v = vec1[i + 3] * vec2[i + 3];
        e += v;
        sum += e;
    }
    for( ; i < len; i++ )
    {
        s += vec1[i] * vec2[i];
    }
    return sum + s;
}

vx_int64
vxCompareBlocksL1_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, vx_int32 len )
{
    vx_int32 i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        vx_int32 v = vec1[i] - vec2[i];
        vx_int32 e = abs(v);

        v = vec1[i + 1] - vec2[i + 1];
        e += abs(v);
        v = vec1[i + 2] - vec2[i + 2];
        e += abs(v);
        v = vec1[i + 3] - vec2[i + 3];
        e += abs(v);
        sum += e;
    }
    for( ; i < len; i++ )
    {
        vx_int32 v = vec1[i] - vec2[i];
        s += abs(v);
    }
    return sum + s;
}

vx_int64
vxCompareBlocksL2_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, vx_int32 len )
{
    vx_int32 i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        vx_int32 v = vec1[i] - vec2[i];
        vx_int32 e = v * v;

        v = vec1[i + 1] - vec2[i + 1];
        e += v * v;
        v = vec1[i + 2] - vec2[i + 2];
        e += v * v;
        v = vec1[i + 3] - vec2[i + 3];
        e += v * v;
        sum += e;
    }
    for( ; i < len; i++ )
    {
        vx_int32 v = vec1[i] - vec2[i];

        s += v * v;
    }
    return sum + s;
}

vx_int64
vxCompareBlocksL2_Square_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, vx_int32 len )
{
    vx_int32 i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        vx_int32 v = vec1[i] * vec2[i];
        vx_int32 e = v * v ;
        v = vec1[i + 1] * vec2[i + 1];
        e += v * v ;
        v = vec1[i + 2] * vec2[i + 2];
        e += v * v ;
        v = vec1[i + 3] * vec2[i + 3];
        e += v * v ;
        sum += e;
    }
    for( ; i < len; i++ )
    {
        vx_int32 v = vec1[i] * vec2[i];
        s += v * v ;
    }
    return sum + s;
}

vx_int32 vx_align(vx_uint32 size)
{
    return (size + 16 + 32 - 1) & -32;
}
void vxCalculateBufferSizes( vx_uint32 src_width, vx_uint32 src_height,
                         vx_uint32 template_width, vx_uint32 template_height,
                         vx_int32 is_centered, vx_int32 is_normed,
                         vx_int32 *imgBufSize, vx_int32 *templBufSize,
                         vx_int32 *sumBufSize, vx_int32 *sqsumBufSize,
                         vx_int32 *resNumBufSize, vx_int32 *resDenomBufSize )
{
    vx_int32 depth = 1;

    *imgBufSize = vx_align( (template_width * src_height + src_width) * depth );
    *templBufSize = vx_align( template_width * template_height * depth );
    *resNumBufSize = vx_align( (src_height - template_height + 1) * sizeof( vx_int64 ));

    if( is_centered || is_normed )
    {
        *sumBufSize = vx_align( src_height * sizeof( vx_float64 ));
        *sqsumBufSize = vx_align( src_height * sizeof( vx_float64 ));
        *resDenomBufSize = vx_align( (src_height - template_height + 1) *
                                  (is_centered + is_normed) * sizeof( vx_float64 ));
    }
}
vx_status vxMatchTemplateGetBufSize( vx_uint32 src_width, vx_uint32 src_height,
                            vx_uint32 template_width, vx_uint32 template_height,
                            vx_int32 *bufferSize, vx_int32 is_centered, vx_int32 is_normed )
{
    vx_int32 imgBufSize = 0, templBufSize = 0, sumBufSize = 0, sqsumBufSize = 0,
        resNumBufSize = 0, resDenomBufSize = 0;

    if( !bufferSize )
        return VX_FAILURE;
    *bufferSize = 0;

    vxCalculateBufferSizes(src_width, src_height, template_width, template_height,
                            is_centered, is_normed,
                            &imgBufSize, &templBufSize,
                            &sumBufSize, &sqsumBufSize, &resNumBufSize, &resDenomBufSize );

    *bufferSize = imgBufSize + templBufSize + sumBufSize + sqsumBufSize +
        resNumBufSize + resDenomBufSize;

    return VX_SUCCESS;
}

vx_status vxMatchTemplateEntry(const vx_image source_image,
                       const vx_uint32 src_width, const vx_uint32 src_height,
                       const vx_image template_image,
                       const vx_uint32 template_width, const vx_uint32 template_height,
                       vx_image output,
                       void *pBuffer,
                       vx_int32 is_centered, vx_int32 is_normed,
                       void **imgBuf, void **templBuf,
                       void **sumBuf, void **sqsumBuf, void **resNum, void **resDenom )
{
    vx_status status = VX_SUCCESS;
    vx_int32 templBufSize = 0, imgBufSize = 0,
             sumBufSize = 0, sqsumBufSize = 0, resNumBufSize = 0, resDenomBufSize = 0;
    vx_int32 i;
    vx_int8 *buffer = (vx_int8 *) pBuffer;

    if( !pBuffer )
        return VX_FAILURE;

    vxCalculateBufferSizes(src_width, src_height, template_width, template_height,
                             is_centered, is_normed,
                             &imgBufSize, &templBufSize,
                             &sumBufSize, &sqsumBufSize, &resNumBufSize, &resDenomBufSize );
    *templBuf = buffer;
    buffer += templBufSize;

    *imgBuf = buffer;
    buffer += imgBufSize;

    *resNum = buffer;
    buffer += resNumBufSize;

    if( is_centered || is_normed )
    {
        if( sumBuf )
            *sumBuf = buffer;
        buffer += sumBufSize;

        if( sqsumBuf )
            *sqsumBuf = buffer;
        buffer += sqsumBufSize;

        if( resDenom )
            *resDenom = buffer;
        buffer += resDenomBufSize;
    }
    void* src;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT, template_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id source_map_id = 0, template_map_id = 0;
    vx_rectangle_t source_rect, template_rect;
    status |= vxGetValidRegionImage(source_image, &source_rect);
    if( status != VX_SUCCESS ) return status;
    src = NULL;
    status = vxMapImagePatch(source_image, &source_rect, 0, &source_map_id, &addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    if( status != VX_SUCCESS ) return status;

    for (i = 0; i < src_height; i++)
    {
        vx_uint8* srcp = vxFormatImagePatchAddress1d(src, i * src_width, &addr);
        memcpy(*imgBuf + i * template_width, srcp, template_width);
    }
    status |= vxUnmapImagePatch(source_image, source_map_id);

    status |= vxGetValidRegionImage(template_image, &template_rect);
    if( status != VX_SUCCESS ) return status;
    src = NULL;
    status = vxMapImagePatch(template_image, &template_rect, 0, &template_map_id, &template_addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    if( status != VX_SUCCESS ) return status;
    for (i = 0; i < template_height; i++)
    {
        vx_uint8* srcp = vxFormatImagePatchAddress1d(src, i * template_width, &template_addr);
        memcpy(*templBuf + i * template_width, srcp, template_width);
    }
    status = vxUnmapImagePatch(template_image, template_map_id);
    return status;
}

vx_status matchTemplate_HAMMING(const vx_image source_image, vx_uint32 src_width, vx_uint32 src_height,
               const vx_image template_image, vx_uint32 template_width, vx_uint32 template_height,
               vx_image result_image, void *pBuffer)
{
    vx_status status = VX_SUCCESS;
    vx_uint8 *imgBuf = 0;
    vx_uint8 *templBuf = 0;
    vx_int64 *resNum = 0;
    vx_int32 winLen = template_width * template_height;
    vx_float64 winCoeff = 1. / (winLen + DBL_EPSILON);
    vx_int32 result_width = src_width - template_width + 1;
    vx_int32 result_height = src_height - template_height + 1;
    vx_int32 x, y;

    vx_status result = vxMatchTemplateEntry(source_image, src_width, src_height,
                                             template_image, template_width, template_height,
                                             result_image, pBuffer,
                                             0, 0,
                                             (void **) &imgBuf, (void **) &templBuf,
                                             0, 0,
                                             (void **) &resNum, 0 );
    if( result != VX_SUCCESS ) return result;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t result_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    vx_rectangle_t rect;
    vx_rectangle_t result_rect;
    vx_uint8 *src = NULL;
    vx_int16 *result_src = NULL;
    vx_size source_x = 0;
    status |= vxGetValidRegionImage(source_image, &rect);
    status |= vxMapImagePatch(source_image, &rect, 0, &map_id, &addr, (void**)&src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;
    status |= vxGetValidRegionImage(result_image, &result_rect);
    status |= vxMapImagePatch(result_image, &result_rect, 0, &result_map_id, &result_addr, (void**)&result_src, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    const vx_uint32 W16 = winLen / 16 * 16;
    const vx_uint32 W8 = winLen / 8 * 8;
    const vx_uint32 src_dim_x = addr.dim_x;
    const vx_uint32 dst_dim_x = result_addr.dim_x;
    const vx_uint32 srcStride_y = addr.stride_y;
    const vx_uint8 dstStride_x = result_addr.stride_x / 2;
    const vx_uint8 dstStride_y = result_addr.stride_y / 2;

    uint16x8_t vZero = vdupq_n_u16(0);

    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;

        source_x = x + template_width - 1;
        vx_uint8 *dst = imgPtr - 1;
        dst += template_width;

        vx_uint32 _x = source_x % src_dim_x;

        vx_uint8* srcp = (vx_uint8 *)src + _x;

        for( y = 0; y < src_height; y++, source_x+=src_width, dst += template_width )
        {
            dst[0] = srcp[0];
            srcp += srcStride_y;
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_uint8 * ptrSrc = (vx_uint8 *)(imgPtr);
            vx_uint8 * ptrTemp = (vx_uint8 *)(templBuf);

            uint32x4_t vSum = vdupq_n_u32(0);

            for (vx_uint32 i = 0; i < W16; i += 16)
            {
                uint8x16_t vSrc = vld1q_u8(ptrSrc);
                uint8x16_t vTemp = vld1q_u8(ptrTemp);
                uint8x16_t vEor = veorq_u8(vSrc, vTemp);

                uint16x8_t vSum0 = vaddl_u8(vget_high_u8(vEor), vget_low_u8(vEor));
                vSum = vaddw_u16(vSum, vget_high_u16(vSum0));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum0));

                ptrSrc += 16;
                ptrTemp += 16;
            }

            for (vx_uint32 i = W16; i < W8; i += 8)
            {
                uint8x8_t vSrc = vld1_u8(ptrSrc); 
                uint8x8_t vTemp = vld1_u8(ptrTemp); 
                uint8x8_t vEor = veor_u8(vSrc, vTemp);
                uint16x8_t vSum1 = vaddw_u8(vZero, vEor);

                vSum = vaddw_u16(vSum, vget_high_u16(vSum1));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum1));
                ptrSrc += 8;
                ptrTemp += 8;
            }

            vx_uint32 sum2 = 0;

            for (vx_uint32 i = W8; i < winLen; i++)
            {
                vx_int32 v = ptrSrc[i] ^ ptrTemp[i];
                sum2 += v;
            }

            vx_uint32 sum0[4];
            vst1q_u32(sum0, vSum);
            sum2 += sum2 + sum0[0] + sum0[1] + sum0[2] + sum0[3];  

            vx_uint32 index = x+y*result_width;
            vx_uint32 _x = index % dst_dim_x;
            vx_uint32 _y = index / dst_dim_x;

            vx_int16 *result_srcp = (vx_int16 *)result_src + _x * dstStride_x + _y * dstStride_y;

            *result_srcp = sum2 * winCoeff;
        }
    }

    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(source_image, map_id);
        status |= vxUnmapImagePatch(result_image, result_map_id);
    }
    return status;
}

vx_status matchTemplate_COMPARE_L1(const vx_image source_image, vx_uint32 src_width, vx_uint32 src_height,
               const vx_image template_image, vx_uint32 template_width, vx_uint32 template_height,
               vx_image result_image, void *pBuffer)
{
    vx_status status = VX_SUCCESS;
    vx_uint8 *imgBuf = 0;
    vx_uint8 *templBuf = 0;
    vx_int64 *resNum = 0;
    vx_int32 winLen = template_width * template_height;
    vx_float64 winCoeff = 1. / (winLen + DBL_EPSILON);
    vx_int32 result_width = src_width - template_width + 1;
    vx_int32 result_height = src_height - template_height + 1;
    vx_int32 x, y;

    vx_status result = vxMatchTemplateEntry(source_image, src_width, src_height,
                                             template_image, template_width, template_height,
                                             result_image, pBuffer,
                                             0, 0,
                                             (void **) &imgBuf, (void **) &templBuf,
                                             0, 0,
                                             (void **) &resNum, 0 );
    if( result != VX_SUCCESS ) return result;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t result_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    vx_rectangle_t rect;
    vx_rectangle_t result_rect;
    vx_uint8 *src = NULL;
    vx_int16 *result_src = NULL;
    vx_size source_x = 0;
    status |= vxGetValidRegionImage(source_image, &rect);
    status |= vxMapImagePatch(source_image, &rect, 0, &map_id, &addr, (void**)&src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    status |= vxGetValidRegionImage(result_image, &result_rect);
    status |= vxMapImagePatch(result_image, &result_rect, 0, &result_map_id, &result_addr, (void**)&result_src, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    const vx_uint32 W16 = winLen / 16 * 16;
    const vx_uint32 W8 = winLen / 8 * 8;
    const vx_uint32 src_dim_x = addr.dim_x;
    const vx_uint32 srcStride_y = addr.stride_y;
    const vx_uint8 dstStride_x = result_addr.stride_x / 2;
    const vx_uint8 dstStride_y = result_addr.stride_y / 2;

    uint16x8_t vZero = vdupq_n_u16(0);
    /* main loop - through x coordinate of the result */

    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;

        source_x = x + template_width - 1;
        vx_uint8 *dst = imgPtr - 1;
        dst += template_width;

        vx_uint32 _x = source_x % src_dim_x;

        vx_uint8* srcp = (vx_uint8 *)src + _x;

        for( y = 0; y < src_height; y++, source_x+=src_width, dst += template_width )
        {
            dst[0] = srcp[0];
            srcp += srcStride_y;
        }

        vx_int16 *result_srcp = (vx_int16 *)result_src + x * dstStride_x;

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_uint8 * ptrSrc = (vx_uint8 *)(imgPtr);
            vx_uint8 * ptrTemp = (vx_uint8 *)(templBuf);

            uint32x4_t vSum = vdupq_n_u32(0);

            for (vx_uint32 i = 0; i < W16; i += 16)
            {
                uint8x16_t vSrc = vld1q_u8(ptrSrc);
                uint8x16_t vTemp = vld1q_u8(ptrTemp);
                uint8x16_t vAbsDiff = vabdq_u8(vSrc, vTemp);

                uint16x8_t vSum0 = vaddl_u8(vget_high_u8(vAbsDiff), vget_low_u8(vAbsDiff));
                vSum = vaddw_u16(vSum, vget_high_u16(vSum0));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum0));

                ptrSrc += 16;
                ptrTemp += 16;
            }

            for (vx_uint32 i = W16; i < W8; i += 8)
            {
                uint8x8_t vSrc = vld1_u8(ptrSrc); 
                uint8x8_t vTemp = vld1_u8(ptrTemp); 
                uint8x8_t vAbsDiff = vabd_u8(vSrc, vTemp);
                uint16x8_t vSum1 = vaddw_u8(vZero, vAbsDiff);

                vSum = vaddw_u16(vSum, vget_high_u16(vSum1));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum1));
                ptrSrc += 8;
                ptrTemp += 8;
            }

            vx_uint32 sum2 = 0;

            for (vx_uint32 i = W8; i < winLen; i++)
            {
                vx_uint8 a = ptrSrc[i];
                vx_uint8 b = ptrTemp[i];

                sum2 += (a > b ? a - b : b - a);
            }

            vx_uint32 sum0[4];
            vst1q_u32(sum0, vSum);
            sum2 += sum2 + sum0[0] + sum0[1] + sum0[2] + sum0[3];  

            *result_srcp = sum2 * winCoeff;

            result_srcp += dstStride_y;
        }
    }

    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(source_image, map_id);
        status |= vxUnmapImagePatch(result_image, result_map_id);
    }

    return status;
}

vx_status matchTemplate_COMPARE_L2(const vx_image source_image, vx_uint32 src_width, vx_uint32 src_height,
               const vx_image template_image, vx_uint32 template_width, vx_uint32 template_height,
               vx_image result_image, void *pBuffer)
{
    vx_status status = VX_SUCCESS;
    vx_uint8 *imgBuf = 0;
    vx_uint8 *templBuf = 0;
    vx_int64 *resNum = 0;
    vx_int32 winLen = template_width * template_height;
    vx_float64 winCoeff = 1. / (winLen + DBL_EPSILON);
    vx_int32 result_width = src_width - template_width + 1;
    vx_int32 result_height = src_height - template_height + 1;
    vx_int32 x, y;

    vx_status result = vxMatchTemplateEntry(source_image, src_width, src_height,
                                             template_image, template_width, template_height,
                                             result_image, pBuffer,
                                             0, 0,
                                             (void **) &imgBuf, (void **) &templBuf,
                                             0, 0,
                                             (void **) &resNum, 0 );
    if( result != VX_SUCCESS ) return result;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t result_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    vx_rectangle_t rect;
    vx_rectangle_t result_rect;
    vx_uint8 *src = NULL;
    vx_int16 *result_src = NULL;
    vx_size source_x = 0;
    status |= vxGetValidRegionImage(source_image, &rect);
    status |= vxMapImagePatch(source_image, &rect, 0, &map_id, &addr, (void**)&src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    status |= vxGetValidRegionImage(result_image, &result_rect);
    status |= vxMapImagePatch(result_image, &result_rect, 0, &result_map_id, &result_addr, (void**)&result_src, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    const vx_uint32 W16 = winLen / 16 * 16;
    const vx_uint32 W8 = winLen / 8 * 8;
    const vx_uint32 src_dim_x = addr.dim_x;
    const vx_uint32 dst_dim_x = result_addr.dim_x;
    const vx_uint32 srcStride_y = addr.stride_y;
    const vx_uint8 dstStride_x = result_addr.stride_x / 2;
    const vx_uint8 dstStride_y = result_addr.stride_y / 2;

    uint16x8_t vZero = vdupq_n_u16(0);

    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;

        source_x = x + template_width - 1;
        vx_uint8 *dst = imgPtr - 1;
        dst += template_width;

        vx_uint32 _x = source_x % src_dim_x;

        vx_uint8* srcp = (vx_uint8 *)src + _x;

        for( y = 0; y < src_height; y++, source_x+=src_width, dst += template_width )
        {
            dst[0] = srcp[0];
            srcp += srcStride_y;
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_uint8 * ptrSrc = (vx_uint8 *)(imgPtr);
            vx_uint8 * ptrTemp = (vx_uint8 *)(templBuf);

            uint32x4_t vSum = vdupq_n_u32(0);

            for (vx_uint32 i = 0; i < W16; i += 16)
            {
                uint8x16_t vSrc = vld1q_u8(ptrSrc);
                uint8x16_t vTemp = vld1q_u8(ptrTemp);
                uint8x16_t vSub = vsubq_u8(vSrc, vTemp);
                uint8x16_t vMul = vmulq_u8(vSub, vSub);

                uint16x8_t vSum0 = vaddl_u8(vget_high_u8(vMul), vget_low_u8(vMul));
                vSum = vaddw_u16(vSum, vget_high_u16(vSum0));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum0));

                ptrSrc += 16;
                ptrTemp += 16;
            }

            for (vx_uint32 i = W16; i < W8; i += 8)
            {
                uint8x8_t vSrc = vld1_u8(ptrSrc); 
                uint8x8_t vTemp = vld1_u8(ptrTemp); 
                uint8x8_t vSub = vsub_u8(vSrc, vTemp);
                uint8x8_t vMul = vmul_u8(vSub, vSub);

                uint16x8_t vSum1 = vaddw_u8(vZero, vMul);

                vSum = vaddw_u16(vSum, vget_high_u16(vSum1));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum1));
                ptrSrc += 8;
                ptrTemp += 8;
            }

            vx_uint32 sum2 = 0;

            for (vx_uint32 i = W8; i < winLen; i++)
            {
                vx_int32 v = ptrSrc[i] - ptrTemp[i];
                sum2 += v * v;
            }

            vx_uint32 sum0[4];
            vst1q_u32(sum0, vSum);
            sum2 += sum2 + sum0[0] + sum0[1] + sum0[2] + sum0[3];  

            vx_uint32 index = x+y*result_width;
            vx_uint32 _x = index % dst_dim_x;
            vx_uint32 _y = index / dst_dim_x;

            vx_int16 *result_srcp = (vx_int16 *)result_src + _x * dstStride_x + _y * dstStride_y;

            *result_srcp = sum2 * winCoeff;
        }
    }

    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(source_image, map_id);
        status |= vxUnmapImagePatch(result_image, result_map_id);
    }
    return status;
}

vx_status matchTemplate_CCORR(const vx_image source_image, vx_uint32 src_width, vx_uint32 src_height,
               const vx_image template_image, vx_uint32 template_width, vx_uint32 template_height,
               vx_image result_image, void *pBuffer)
{
    vx_status status = VX_SUCCESS;
    vx_uint8 *imgBuf = 0;
    vx_uint8 *templBuf = 0;
    vx_int64 *resNum = 0;
    vx_int32 winLen = template_width * template_height;
    vx_float64 winCoeff = 1. / (winLen + DBL_EPSILON);
    vx_int32 result_width = src_width - template_width + 1;
    vx_int32 result_height = src_height - template_height + 1;
    vx_int32 x, y;

    vx_status result = vxMatchTemplateEntry(source_image, src_width, src_height,
                                             template_image, template_width, template_height,
                                             result_image, pBuffer,
                                             0, 0,
                                             (void **) &imgBuf, (void **) &templBuf,
                                             0, 0,
                                             (void **) &resNum, 0 );
    if( result != VX_SUCCESS ) return result;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    vx_rectangle_t rect;
    vx_rectangle_t result_rect;
    vx_uint8 *src = NULL;
    vx_int16 *result_src = NULL;
    vx_size source_x = 0;
    status |= vxGetValidRegionImage(source_image, &rect);
    status |= vxMapImagePatch(source_image, &rect, 0, &map_id, &addr, (void**)&src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    vx_imagepatch_addressing_t result_addr = VX_IMAGEPATCH_ADDR_INIT;
    status |= vxGetValidRegionImage(result_image, &result_rect);
    status |= vxMapImagePatch(result_image, &result_rect, 0, &result_map_id, &result_addr, (void**)&result_src, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    const vx_uint32 W16 = winLen / 16 * 16;
    const vx_uint32 W8 = winLen / 8 * 8;
    const vx_uint32 src_dim_x = addr.dim_x;
    const vx_uint32 dst_dim_x = result_addr.dim_x;
    const vx_uint32 srcStride_y = addr.stride_y;
    const vx_uint8 dstStride_x = result_addr.stride_x / 2;
    const vx_uint8 dstStride_y = result_addr.stride_y / 2;

    uint16x8_t vZero = vdupq_n_u16(0);

    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;

        source_x = x + template_width - 1;
        vx_uint8 *dst = imgPtr - 1;
        dst += template_width;

        vx_uint32 _x = source_x % src_dim_x;

        vx_uint8* srcp = (vx_uint8 *)src + _x;

        for( y = 0; y < src_height; y++, source_x+=src_width, dst += template_width )
        {
            dst[0] = srcp[0];
            srcp += srcStride_y;
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_uint8 * ptrSrc = (vx_uint8 *)(imgPtr);
            vx_uint8 * ptrTemp = (vx_uint8 *)(templBuf);

            uint32x4_t vSum = vdupq_n_u32(0);

            const vx_uint32 W8 = winLen / 8 * 8;

            uint16x8_t vZero = vdupq_n_u16(0);

            for (vx_uint32 i = 0; i < W8; i += 8)
            {
                uint8x8_t vSrc = vld1_u8(ptrSrc); 
                uint8x8_t vTemp = vld1_u8(ptrTemp); 
                uint16x8_t vMul = vmull_u8(vSrc, vTemp);

                uint16x8_t vSum0 = vaddq_u16(vZero, vMul);
                vSum = vaddw_u16(vSum, vget_high_u16(vSum0));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum0));

                ptrSrc += 8;
                ptrTemp += 8;
            }

            vx_uint32 sum2 = 0;

            for (vx_uint32 i = W8; i < winLen; i++)
            {
                vx_int32 v = ptrSrc[i] * ptrTemp[i];
                sum2 += v;
            }

            vx_uint32 sum0[4];
            vst1q_u32(sum0, vSum);
            sum2 += sum2 + sum0[0] + sum0[1] + sum0[2] + sum0[3];  

            vx_uint32 index = x+y*result_width;
            vx_uint32 _x = index % dst_dim_x;
            vx_uint32 _y = index / dst_dim_x;

            vx_int16 *result_srcp = (vx_int16 *)result_src + _x * dstStride_x + _y * dstStride_y;

            *result_srcp = sum2 * winCoeff;
        }
    }


    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(source_image, map_id);
        status |= vxUnmapImagePatch(result_image, result_map_id);
    }
    return status;
}


vx_status matchTemplate_COMPARE_L2_NORM(const vx_image source_image, vx_uint32 src_width, vx_uint32 src_height,
               const vx_image template_image, vx_uint32 template_width, vx_uint32 template_height,
               vx_image result_image, void *pBuffer)
{
    vx_status status = VX_SUCCESS;
    vx_uint8 *imgBuf = 0;
    vx_uint8 *templBuf = 0;
    vx_int64 *sqsumBuf = 0;
    vx_int64 *resNum = 0;
    vx_int32 winLen = template_width * template_height;
    vx_int32 result_width = src_width - template_width + 1;
    vx_int32 result_height = src_height - template_height + 1;
    vx_int32 x, y;

    vx_status result = vxMatchTemplateEntry(source_image, src_width, src_height,
                                             template_image, template_width, template_height,
                                             result_image, pBuffer,
                                             0, 1,
                                             (void **) &imgBuf, (void **) &templBuf,
                                             0, (void **)&sqsumBuf,
                                             (void **) &resNum, 0 );
    if( result != VX_SUCCESS ) return result;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t result_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    vx_rectangle_t rect;
    vx_rectangle_t result_rect;
    vx_uint8 *src = NULL;
    vx_int16 *result_src = NULL;
    vx_size source_x = 0;
    status |= vxGetValidRegionImage(source_image, &rect);
    status |= vxMapImagePatch(source_image, &rect, 0, &map_id, &addr, (void**)&src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS )
        return status;

    status |= vxGetValidRegionImage(result_image, &result_rect);
    status |= vxMapImagePatch(result_image, &result_rect, 0, &result_map_id, &result_addr, (void**)&result_src, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS )
        return status;

    const vx_uint32 W16 = winLen / 16 * 16;
    const vx_uint32 W8 = winLen / 8 * 8;
    const vx_uint32 src_dim_x = addr.dim_x;
    const vx_uint32 dst_dim_x = result_addr.dim_x;
    const vx_uint32 srcStride_y = addr.stride_y;
    const vx_uint8 dstStride_x = result_addr.stride_x / 2;
    const vx_uint8 dstStride_y = result_addr.stride_y / 2;

    uint16x8_t vZero = vdupq_n_u16(0);

    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;

        source_x = x + template_width - 1;
        vx_uint8 *dst = imgPtr - 1;
        dst += template_width;

        vx_uint32 _x = source_x % src_dim_x;

        vx_uint8* srcp = (vx_uint8 *)src + _x;

        for( y = 0; y < src_height; y++, source_x+=src_width, dst += template_width )
        {
            dst[0] = srcp[0];
            srcp += srcStride_y;
        }

        vx_int16 *result_srcp = (vx_int16 *)result_src + x * dstStride_x;

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_uint8 * ptrSrc = (vx_uint8 *)(imgPtr);
            vx_uint8 * ptrTemp = (vx_uint8 *)(templBuf);

            uint32x4_t vSum_L2 = vdupq_n_u32(0);
            uint32x4_t vSum = vdupq_n_u32(0);

            for (vx_uint32 i = 0; i < W16; i += 16)
            {
                uint8x16_t vSrc = vld1q_u8(ptrSrc);
                uint8x16_t vTemp = vld1q_u8(ptrTemp);

                // res_L2
                uint8x16_t vSub_L2 = vsubq_u8(vSrc, vTemp);
                uint8x16_t vMul_L2 = vmulq_u8(vSub_L2, vSub_L2);
                uint16x8_t vSum0_L2 = vaddl_u8(vget_high_u8(vMul_L2), vget_low_u8(vMul_L2));
                vSum_L2 = vaddw_u16(vSum_L2, vget_high_u16(vSum0_L2));
                vSum_L2 = vaddw_u16(vSum_L2, vget_low_u16(vSum0_L2));


                // res
                uint8x16_t vMul = vmulq_u8(vSrc, vTemp);
                uint8x16_t vMul_2 = vmulq_u8(vMul, vMul);
                uint16x8_t vSum0 = vaddl_u8(vget_high_u8(vMul_2), vget_low_u8(vMul_2));
                vSum = vaddw_u16(vSum, vget_high_u16(vSum0));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum0));

                ptrSrc += 16;
                ptrTemp += 16;
            }

            for (vx_uint32 i = W16; i < W8; i += 8)
            {
                uint8x8_t vSrc = vld1_u8(ptrSrc); 
                uint8x8_t vTemp = vld1_u8(ptrTemp); 

                // res_L2
                uint8x8_t vSub_L2 = vsub_u8(vSrc, vTemp);
                uint8x8_t vMul_L2 = vmul_u8(vSub_L2, vSub_L2);
                uint16x8_t vSum1_L2 = vaddw_u8(vZero, vMul_L2);
                vSum_L2 = vaddw_u16(vSum_L2, vget_high_u16(vSum1_L2));
                vSum_L2 = vaddw_u16(vSum_L2, vget_low_u16(vSum1_L2));

                // res
                uint8x8_t vMul = vsub_u8(vSrc, vTemp);
                uint8x8_t vMul_2 = vmul_u8(vMul, vMul);
                uint16x8_t vSum1 = vaddw_u8(vZero, vMul_2);
                vSum = vaddw_u16(vSum, vget_high_u16(vSum1));
                vSum = vaddw_u16(vSum, vget_low_u16(vSum1));

                ptrSrc += 8;
                ptrTemp += 8;
            }

            vx_uint32 sum2_L2 = 0;
            vx_uint32 sum2 = 0;

            for (vx_uint32 i = W8; i < winLen; i++)
            {
                vx_int32 v_L2 = ptrSrc[i] - ptrTemp[i];
                sum2_L2 += v_L2 * v_L2;

                vx_int32 v = ptrSrc[i] * ptrTemp[i];
                sum2 += v * v;
            }

            vx_uint32 sum0_L2[4];
            vst1q_u32(sum0_L2, vSum_L2);
            sum2_L2 += sum2_L2 + sum0_L2[0] + sum0_L2[1] + sum0_L2[2] + sum0_L2[3];  

            vx_uint32 sum0[4];
            vst1q_u32(sum0, vSum);
            sum2 += sum2 + sum0[0] + sum0[1] + sum0[2] + sum0[3];  

            *result_srcp = sum2 / sqrt(sum2_L2);

            result_srcp += dstStride_y;
        }
    }

    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(source_image, map_id);
        status |= vxUnmapImagePatch(result_image, result_map_id);
    }
    return status;
}


static vx_uint32 neonCrossCorr_8u(const vx_uint8 * imgPtr, const vx_uint8 * templBuf, vx_int32 winLen)
{
    vx_uint8 * ptrSrc = (vx_uint8 *)(imgPtr);
    vx_uint8 * ptrTemp = (vx_uint8 *)(templBuf);

    const vx_uint32 W8 = winLen / 8 * 8;

    uint32x4_t vSum = vdupq_n_u32(0);
    uint16x8_t vZero = vdupq_n_u16(0);
    vx_uint32 sum2 = 0;

    for (vx_uint32 i = 0; i < W8; i += 8)
    {
        uint8x8_t vSrc = vld1_u8(ptrSrc); 
        uint8x8_t vTemp = vld1_u8(ptrTemp); 
        uint16x8_t vMul = vmull_u8(vSrc, vTemp);

        uint16x8_t vSum0 = vaddq_u16(vZero, vMul);
        vSum = vaddw_u16(vSum, vget_high_u16(vSum0));
        vSum = vaddw_u16(vSum, vget_low_u16(vSum0));

        ptrSrc += 8;
        ptrTemp += 8;
    }

    for (vx_uint32 i = W8; i < winLen; i++)
    {
        vx_int32 v = ptrSrc[i] * ptrTemp[i];
        sum2 += v;
    }

    vx_uint32 sum0[4];
    vst1q_u32(sum0, vSum);
    sum2 += sum2 + sum0[0] + sum0[1] + sum0[2] + sum0[3];  

    return sum2;
}

vx_status matchTemplate_COMPARE_CCORR_NORM(const vx_image source_image, vx_uint32 src_width, vx_uint32 src_height,
               const vx_image template_image, vx_uint32 template_width, vx_uint32 template_height,
               vx_image result_image, void *pBuffer)
{
    vx_status status = VX_SUCCESS;
    vx_uint8 *imgBuf = 0;
    vx_uint8 *templBuf = 0;
    vx_int64 *sqsumBuf = 0;
    vx_int64 *resNum = 0;
    vx_int64 *resDenom = 0;
    vx_float64 templCoeff = 0;
    vx_int32 winLen = template_width * template_height;
    vx_int32 result_width = src_width - template_width + 1;
    vx_int32 result_height = src_height - template_height + 1;
    vx_int32 x, y;

    vx_status result = vxMatchTemplateEntry(source_image, src_width, src_height,
                                             template_image, template_width, template_height,
                                             result_image, pBuffer,
                                             0, 1,
                                             (void **) &imgBuf, (void **) &templBuf,
                                             0, (void **)&sqsumBuf,
                                             (void **) &resNum, (void **) &resDenom );
    if( result != VX_SUCCESS ) return result;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    vx_rectangle_t rect;
    vx_rectangle_t result_rect;
    vx_uint8 *src = NULL;
    vx_int16 *result_src = NULL;
    vx_size source_x = 0;
    status |= vxGetValidRegionImage(source_image, &rect);
    status |= vxMapImagePatch(source_image, &rect, 0, &map_id, &addr, (void**)&src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    vx_imagepatch_addressing_t result_addr = VX_IMAGEPATCH_ADDR_INIT;
    status |= vxGetValidRegionImage(result_image, &result_rect);
    status |= vxMapImagePatch(result_image, &result_rect, 0, &result_map_id, &result_addr, (void**)&result_src, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if( status != VX_SUCCESS ) return status;

    const vx_uint32 W16 = winLen / 16 * 16;
    const vx_uint32 W8 = winLen / 8 * 8;
    const vx_uint32 src_dim_x = addr.dim_x;
    const vx_uint32 dst_dim_x = result_addr.dim_x;
    const vx_uint32 srcStride_y = addr.stride_y;
    const vx_uint8 dstStride_x = result_addr.stride_x / 2;
    const vx_uint8 dstStride_y = result_addr.stride_y / 2;

    /* calc common statistics for template and image */
    {
        //compute sqrt{sum_{grave{x},grave{y}}^{w,h} T(grave{x},grave{y})^2}
        vx_uint8 *rowPtr = (vx_uint8 *) imgBuf;
        vx_int64 templSqsum =neonCrossCorr_8u( templBuf, templBuf, winLen );

        templCoeff = (vx_float64) templSqsum;
        templCoeff = vxInvSqrt64d( fabs( templCoeff ) + FLT_EPSILON );

        //compute sum_{grave{x},grave{y}}^{w,h} I(x+grave{x},y+grave{y})^2
        for( y = 0; y < src_height; y++, rowPtr += template_width )
        {
            sqsumBuf[y] = neonCrossCorr_8u( rowPtr, rowPtr, template_width);
        }

    }

    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_int64 sqsum = 0;
        vx_uint8 *imgPtr = imgBuf + x;

        source_x = x + template_width - 1;
        vx_uint8 *dst = imgPtr - 1;
        vx_int32 out_val = dst[0];
        dst += template_width;

        vx_uint32 _x = source_x % src_dim_x;

        vx_uint8* srcp = (vx_uint8 *)src + _x;

        for( y = 0; y < src_height; y++, source_x+=src_width, dst += template_width )
        {
            vx_int32 in_val = srcp[0];
            sqsumBuf[y] += (in_val - out_val) * (in_val + out_val);
            out_val = dst[0];
            dst[0] = (vx_uint8) in_val;
            srcp += srcStride_y;
        }

        //for specific x, compute sqsum from row 0 to row template_height-1
        for( y = 0; y < template_height; y++ )
        {
            sqsum += sqsumBuf[y];
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            //compute sum_{grave{x},grave{y}}^{w,h} T(grave{x},grave{y}) * I(x+grave{x},y+grave{y})
            vx_int64 res = neonCrossCorr_8u( imgPtr, templBuf, winLen );

            if( y > 0 )
            {
                //move sqsum down
                sqsum -= sqsumBuf[y - 1];
                sqsum += sqsumBuf[y + template_height - 1];
            }

            resNum[y] = res;
            resDenom[y] = sqsum;

            vx_float64 res1 = ((vx_float64) resNum[y]) * templCoeff *
                vxInvSqrt64d( fabs( (vx_float64) resDenom[y] ) + FLT_EPSILON );

            res1 *= pow(2, 15);
            //vx_int16 *result_srcp = vxFormatImagePatchAddress1d(result_src, x+y*result_width, &result_addr);

            vx_uint32 index = x+y*result_width;
            vx_uint32 _x = index % dst_dim_x;
            vx_uint32 _y = index / dst_dim_x;

            vx_int16 *result_srcp = (vx_int16 *)result_src + _x * dstStride_x + _y * dstStride_y;

            *result_srcp = (vx_int16)res1;
        }
    }

    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(source_image, map_id);
        status |= vxUnmapImagePatch(result_image, result_map_id);
    }
    return status;
}

vx_status vxMatchTemplate(vx_image src, vx_image template_image, vx_scalar matchingMethod, vx_image output)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 src_width;
    vx_uint32 src_height;
    vx_uint32 template_width;
    vx_uint32 template_height;
    vxQueryImage(src, VX_IMAGE_WIDTH, &src_width, sizeof(vx_uint32));
    vxQueryImage(src, VX_IMAGE_HEIGHT, &src_height, sizeof(vx_uint32));
    vxQueryImage(template_image, VX_IMAGE_WIDTH, &template_width, sizeof(vx_uint32));
    vxQueryImage(template_image, VX_IMAGE_HEIGHT, &template_height, sizeof(vx_uint32));

    vx_int32 bufferSize;
    vxMatchTemplateGetBufSize(src_width, src_height, template_width, template_height, &bufferSize, 1, 1 );

    void* pBuffer = malloc( bufferSize );
    vx_int32 matchMethod = 0;
    vxCopyScalar(matchingMethod, &matchMethod, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    switch(matchMethod)
    {
        case VX_COMPARE_HAMMING:
            {
                matchTemplate_HAMMING(src, src_width, src_height,
                        template_image, template_width, template_height,
                        output, pBuffer);
                break;
            }
        case VX_COMPARE_L1:
            {
                matchTemplate_COMPARE_L1(src, src_width, src_height,
                        template_image, template_width, template_height,
                        output, pBuffer);
                break;
            }
        case VX_COMPARE_L2:
            {
                matchTemplate_COMPARE_L2(src, src_width, src_height,
                        template_image, template_width, template_height,
                        output, pBuffer);
                break;
            }
        case VX_COMPARE_CCORR:
            {
                matchTemplate_CCORR(src, src_width, src_height,
                        template_image, template_width, template_height,
                        output, pBuffer);
                break;
            }
        case VX_COMPARE_L2_NORM:
            {
                matchTemplate_COMPARE_L2_NORM(src, src_width, src_height,
                        template_image, template_width, template_height,
                        output, pBuffer);
                break;
            }
        case VX_COMPARE_CCORR_NORM:
            {
                matchTemplate_COMPARE_CCORR_NORM(src, src_width, src_height,
                        template_image, template_width, template_height,
                        output, pBuffer);
                break;
            }
    }
    free(pBuffer);
    return status;
}

