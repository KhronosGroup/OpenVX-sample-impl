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
#include <c_model.h>

#define _SQRT_MAGIC     0xbe6f0000
/* calculates 1/sqrt(val) */
double
vxInvSqrt64d( double arg )
{
    double x, y;

    /*
        Warning: the following code makes assumptions
        about the format of float.
    */
    union {
        float t;
        vx_uint32 u;
    } floatbits;

    floatbits.t = (float) arg;
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
vxCompareBlocksHamming_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, int len )
{
    int i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        int v = vec1[i] ^ vec2[i];
        int e = v;

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
        int v = vec1[i] ^ vec2[i];
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
vxCompareBlocksL1_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, int len )
{
    int i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        int v = vec1[i] - vec2[i];
        int e = abs(v);

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
        int v = vec1[i] - vec2[i];
        s += abs(v);
    }
    return sum + s;
}

vx_int64
vxCompareBlocksL2_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, int len )
{
    int i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        int v = vec1[i] - vec2[i];
        int e = v * v;

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
        int v = vec1[i] - vec2[i];

        s += v * v;
    }
    return sum + s;
}

vx_int64
vxCompareBlocksL2_Square_8u( const vx_uint8 * vec1, const vx_uint8 * vec2, int len )
{
    int i, s = 0;
    vx_int64 sum = 0;
    for( i = 0; i <= len - 4; i += 4 )
    {
        int v = vec1[i] * vec2[i];
        int e = v * v ;
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
        int v = vec1[i] * vec2[i];
        s += v * v ;
    }
    return sum + s;
}

int vx_align(vx_uint32 size)
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
                            int *bufferSize, int is_centered, int is_normed )
{
    int imgBufSize = 0, templBufSize = 0, sumBufSize = 0, sqsumBufSize = 0,
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
    vx_uint32 i;
    (void)output;

    char *buffer = (char *) pBuffer;

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
        memcpy((vx_uint8*)(*imgBuf) + i * template_width, srcp, template_width);
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
        memcpy((vx_uint8*)(*templBuf) + i * template_width, srcp, template_width);
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
    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;
        /* update sums and image band buffer */
        if( x > 0 )
        {
            source_x = x + template_width - 1;
            vx_uint8 *dst = imgPtr - 1;
            dst += template_width;
            for( y = 0; y < (vx_int32)src_height; y++, source_x+=src_width, dst += template_width )
            {
                vx_uint8* srcp = vxFormatImagePatchAddress1d(src, (vx_int32)source_x, &addr);
                dst[0] = srcp[0];
            }
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            resNum[y] = vxCompareBlocksHamming_8u( imgPtr, templBuf, winLen );
        }

        for( y = 0; y < result_height; y++ )
        {
            vx_float64 res = winCoeff * ((vx_float64) resNum[y]);
            vx_int16 *result_srcp = vxFormatImagePatchAddress1d(result_src, x+y*result_width, &result_addr);
            *result_srcp = (vx_int16)res;
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
    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;
        /* update sums and image band buffer */
        if( x > 0 )
        {
            source_x = x + template_width - 1;
            vx_uint8 *dst = imgPtr - 1;
            dst += template_width;

            for( y = 0; y < (vx_int32)src_height; y++, source_x+=src_width, dst += template_width )
            {
                vx_uint8* srcp = vxFormatImagePatchAddress1d(src, (vx_int32) source_x, &addr);
                dst[0] = srcp[0];
            }
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_int64 res = vxCompareBlocksL1_8u( imgPtr, templBuf, winLen );
            resNum[y] = res;
        }

        for( y = 0; y < result_height; y++ )
        {
            vx_float64 res = winCoeff * ((vx_float64)resNum[y]);
            vx_int16 *result_srcp = vxFormatImagePatchAddress2d(result_src, x, y, &result_addr);
            *result_srcp = (vx_int16)res;
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
    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;
        /* update sums and image band buffer */
        if( x > 0 )
        {
            source_x = x + template_width - 1;
            vx_uint8 *dst = imgPtr - 1;
            dst += template_width;

            for( y = 0; y < (vx_int32)src_height; y++, source_x += src_width, dst += template_width )
            {
                vx_uint8* srcp = vxFormatImagePatchAddress1d(src, (vx_int32)source_x, &addr);
                dst[0] = srcp[0];
            }
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_int64 res = vxCompareBlocksL2_8u(imgPtr, templBuf, winLen);
            resNum[y] = res;
        }

        for( y = 0; y < result_height; y++ )
        {
            vx_int64 res = (vx_int64)(winCoeff * ((vx_float64) resNum[y]));
            vx_int16 *result_srcp = vxFormatImagePatchAddress1d(result_src, x+y*result_width, &result_addr);
            *result_srcp = (vx_int16)res;
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

    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;
        /* update sums and image band buffer */
        if( x > 0 )
        {
            source_x = x + template_width - 1;
            vx_uint8 *dst = imgPtr - 1;
            dst += template_width;

            for( y = 0; y < (vx_int32)src_height; y++, source_x+=src_width, dst += template_width )
            {
                vx_uint8* srcp = vxFormatImagePatchAddress1d(src, (vx_uint32)source_x, &addr);
                dst[0] = srcp[0];
            }
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            resNum[y] = vxCrossCorr_8u( imgPtr, templBuf, winLen );
        }

        for( y = 0; y < result_height; y++ )
        {
            vx_int64 res = (vx_int64)(winCoeff * ((vx_float64) resNum[y]));
            vx_int16 *result_srcp = vxFormatImagePatchAddress1d(result_src, x+y*result_width, &result_addr);
            *result_srcp = (vx_int16)res;
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
    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_uint8 *imgPtr = imgBuf + x;
        /* update sums and image band buffer */
        if( x > 0 )
        {
            source_x = x + template_width - 1;
            vx_uint8 *dst = imgPtr - 1;

            dst += template_width;

            for( y = 0; y < (vx_int32)src_height; y++, source_x+=src_width, dst += template_width )
            {
                vx_uint8* srcp = vxFormatImagePatchAddress1d(src, (vx_uint32)source_x, &addr);
                dst[0] = srcp[0];
            }
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            vx_int64 res_L2 =  vxCompareBlocksL2_8u(imgPtr, templBuf, winLen);
            vx_int64 res =  vxCompareBlocksL2_Square_8u(imgPtr, templBuf, winLen);
            resNum[y] = res;
            sqsumBuf[y] = res_L2;
        }

        for( y = 0; y < result_height; y++ )
        {
            vx_float64 res = ((vx_float64)sqsumBuf[y]/sqrt((vx_float64)resNum[y]));
            vx_int16 *result_srcp = vxFormatImagePatchAddress2d(result_src, x, y, &result_addr);
            *result_srcp = (vx_int16)res;
        }
    }
    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(source_image, map_id);
        status |= vxUnmapImagePatch(result_image, result_map_id);
    }
    return status;
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

    /* calc common statistics for template and image */
    {
        //compute sqrt{sum_{grave{x},grave{y}}^{w,h} T(grave{x},grave{y})^2}
        vx_uint8 *rowPtr = (vx_uint8 *) imgBuf;
        vx_int64 templSqsum =vxCrossCorr_8u( templBuf, templBuf, winLen );

        templCoeff = (vx_float64) templSqsum;
        templCoeff = vxInvSqrt64d( fabs( templCoeff ) + FLT_EPSILON );

        //compute sum_{grave{x},grave{y}}^{w,h} I(x+grave{x},y+grave{y})^2
        for( y = 0; y < (vx_int32)src_height; y++, rowPtr += template_width )
        {
            sqsumBuf[y] = vxCrossCorr_8u( rowPtr, rowPtr, template_width );
        }
    }

    /* main loop - through x coordinate of the result */
    for( x = 0; x < result_width; x++ )
    {
        vx_int64 sqsum = 0;
        vx_uint8 *imgPtr = imgBuf + x;
        /* update sums and image band buffer */
        if( x > 0 )
        {
            source_x = x + template_width - 1;
            vx_uint8 *dst = imgPtr - 1;
            vx_int32 out_val = dst[0];
            dst += template_width;

            for( y = 0; y < (vx_int32)src_height; y++, source_x+=src_width, dst += template_width )
            {
                vx_uint8* srcp = vxFormatImagePatchAddress1d(src, (vx_uint32)source_x, &addr);

                vx_int32 in_val = srcp[0];
                sqsumBuf[y] += (in_val - out_val) * (in_val + out_val);
                out_val = dst[0];
                dst[0] = (vx_uint8) in_val;
            }
        }
        //for specific x, compute sqsum from row 0 to row template_height-1
        for( y = 0; y < (vx_int32)template_height; y++ )
        {
            sqsum += sqsumBuf[y];
        }

        for( y = 0; y < result_height; y++, imgPtr += template_width )
        {
            //compute sum_{grave{x},grave{y}}^{w,h} T(grave{x},grave{y}) * I(x+grave{x},y+grave{y})
            vx_int64 res = vxCrossCorr_8u( imgPtr, templBuf, winLen );
            if( y > 0 )
            {
                //move sqsum down
                sqsum -= sqsumBuf[y - 1];
                sqsum += sqsumBuf[y + template_height - 1];
            }
            resNum[y] = res;
            resDenom[y] = sqsum;
        }

        for( y = 0; y < result_height; y++ )
        {
            vx_float64 res = ((vx_float64) resNum[y]) * templCoeff *
                vxInvSqrt64d( fabs( (vx_float64) resDenom[y] ) + FLT_EPSILON );

            res *= pow(2, 15);
            vx_int16 *result_srcp = vxFormatImagePatchAddress1d(result_src, x+y*result_width, &result_addr);
            *result_srcp = (vx_int16)res;
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

    int bufferSize;
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

