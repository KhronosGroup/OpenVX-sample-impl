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

#include <arm_neon.h>
#include <tiling.h>
#include <string.h>

void box3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_tile_t *out = (vx_tile_t *)parameters[1];
    uint16x8_t vPrv[3];
    uint16x8_t vCur[3];
    uint16x8_t vNxt[3];
    int32x4_t out0 = vdupq_n_s32(0);
    int32x4_t out1 = out0;
    int32x4_t out2 = out0;
    int32x4_t out3 = out0;
    uint8_t szTmpData[8];
    float32x4_t oneovernine = vdupq_n_f32(1.0f / 9.0f);
    uint8_t *dstTmp;
    uint8_t *src = in->base[0] + in->tile_y * in->image.width;
	if (in->tile_y > 0)
	{
		src = in->base[0] + (in->tile_y - 1) * in->image.width;
	}
    uint8_t *dst = out->base[0] + out->tile_y * out->image.width;
    if (in->tile_x == 0)
    {
        vPrv[0] = vPrv[1] = vPrv[2] = vdupq_n_u16(0);
    }
    for (x = 0; x < in->tile_block.width; x+=8)
    {
        dstTmp = dst + out->tile_x + x;
        if (0 != in->tile_x || 0 != x)
        {
            for (y = 0; y < 3; y++)
            {
                vPrv[y] = vmovl_u8(vld1_u8(src + y * in->image.width + in->tile_x + x - 8));
                vCur[y] = vmovl_u8(vld1_u8(src + y * in->image.width + in->tile_x + x));
            }
        }
        else
        {
            for (y = 0; y < 3; y++)
            {
                vCur[y] = vmovl_u8(vld1_u8(src + y * in->image.width + in->tile_x + x));
            }
        }

        if ((in->tile_x + x + 16) < in->image.width)
        {
            for (y = 0; y < 3; y++)
            {
                vNxt[y] = vmovl_u8(vld1_u8(src + y * in->image.width + in->tile_x + x + 8));
            }
        }
        else
        {
            uint32_t leftCnt = in->image.width - (in->tile_x + x + 8);
            uint32_t cpCnt = leftCnt > 8 ? 8 : leftCnt;
            for (y = 0; y < 3; y++)
            {
                memcpy(szTmpData, src + y * in->image.width + in->tile_x + x + 8, cpCnt);
                memset(&szTmpData[cpCnt], 0, 8 - cpCnt);
                vNxt[y] = vmovl_u8(vld1_u8(szTmpData));
            }
        }

        uint8_t tmpIdx = 1;
		y = out->tile_y == 0 ? 1 : out->tile_y;
		vx_int32 maxY = (out->image.height == (out->tile_y + out->tile_block.height)) ? out->image.height - 1 : (out->tile_y + out->tile_block.height);
		for (; y < maxY; y++, tmpIdx++)
        {
            uint16x8_t vSum = vdupq_n_u16(0);
            uint16x8_t vTmp;
            vSum = vaddq_u16(vCur[0], vCur[1]);
            vSum = vaddq_u16(vSum, vCur[2]);

            vTmp = vextq_u16(vPrv[0], vCur[0], 7);
            vSum = vaddq_u16(vTmp, vSum);
            vTmp = vextq_u16(vCur[0], vNxt[0], 1);
            vSum = vaddq_u16(vTmp, vSum);

            vTmp = vextq_u16(vPrv[1], vCur[1], 7);
            vSum = vaddq_u16(vTmp, vSum);
            vTmp = vextq_u16(vCur[1], vNxt[1], 1);
            vSum = vaddq_u16(vTmp, vSum);

            vTmp = vextq_u16(vPrv[2], vCur[2], 7);
            vSum = vaddq_u16(vTmp, vSum);
            vTmp = vextq_u16(vCur[2], vNxt[2], 1);
            vSum = vaddq_u16(vTmp, vSum);

            float32x4_t outfloathigh = vcvtq_f32_s32(vmovl_s16(vreinterpret_s16_u16(vget_high_u16(vSum))));
            float32x4_t outfloatlow  = vcvtq_f32_s32(vmovl_s16(vreinterpret_s16_u16(vget_low_u16(vSum))));

            outfloathigh = vmulq_f32(outfloathigh, oneovernine);
            outfloatlow  = vmulq_f32(outfloatlow, oneovernine);

            int16x8_t vOut = vcombine_s16(vqmovn_s32(vcvtq_s32_f32(outfloatlow)),
                               vqmovn_s32(vcvtq_s32_f32(outfloathigh)));

            //uint8_t *dstTmp1 = dstTmp + y * out->image.width;
			uint8x8_t vOut8 = vqmovun_s16(vOut);
			vst1_u8(out->base[0] + y * out->image.width + out->tile_x + x, vOut8);

            //swap data and acquire next data
            for (uint8_t convIdx = 0; convIdx < 2; convIdx++)
            {
                vPrv[convIdx] = vPrv[convIdx + 1];
                vCur[convIdx] = vCur[convIdx + 1];
                vNxt[convIdx] = vNxt[convIdx + 1];
            }

            if ((y + 2) < out->image.height)
            {
                if (0 != in->tile_x || 0 != x)
                {
                    vPrv[2] = vmovl_u8(vld1_u8(in->base[0] + (y + 2) * in->image.width + in->tile_x + x - 8));
                }
                else
                {
                    vPrv[2] = vdupq_n_u16(0);
                }
                vCur[2] = vmovl_u8(vld1_u8(in->base[0] + (y + 2) * in->image.width + in->tile_x + x));

                if ((in->tile_x + x + 16) < in->image.width)
                {
                    vNxt[2] = vmovl_u8(vld1_u8(in->base[0] + (y + 2) * in->image.width + in->tile_x + x + 8));
                }
                else
                {
                    uint32_t leftCnt = in->image.width - (in->tile_x + x + 8);
                    uint32_t cpCnt = leftCnt > 8 ? 8 : leftCnt;

                    memcpy(szTmpData, in->base[0] + (y + 2) * in->image.width + in->tile_x + x + 8, cpCnt);
                    memset(&szTmpData[cpCnt], 0, 8 - cpCnt);
                    vNxt[2] = vmovl_u8(vld1_u8(szTmpData));
                }
            }
        }
    }
}


void box3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_tile_t *out = (vx_tile_t *)parameters[1];

    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;

    if (ty == 0 && tx == 0)
    {
        for (y = 1; y < vxTileHeight(out, 0); y++)
        {
            for (x = 1; x < vxTileWidth(out, 0); x++)
            {
                vx_int32 j, i;
                vx_uint32 sum = 0;
                vx_uint32 count = 0;
                for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)
                {
                    for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)
                    {
                        sum += vxImagePixel(vx_uint8, in, 0, x, y, i, j);
                        count++;
                    }
                }
                sum /= count;
                if (sum > 255)
                    sum = 255;
                vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
            }
        }
    }
    else
    {
        for (y = 1; y < ty; y++)
        {
            for (x = tx; x < vxTileWidth(out, 0); x++)
            {
                vx_int32 j, i;
                vx_uint32 sum = 0;
                vx_uint32 count = 0;
                for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)
                {
                    for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)
                    {

                        sum += vxImagePixel(vx_uint8, in, 0, x, y, i, j);
                        count++;
                    }
                }
                sum /= count;
                if (sum > 255)
                    sum = 255;
                vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
            }
        }

        for (y = ty; y < vxTileHeight(out, 0); y++)
        {
            for (x = 1; x < vxTileWidth(out, 0); x++)
            {
                vx_int32 j, i;
                vx_uint32 sum = 0;
                vx_uint32 count = 0;
                for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)
                {
                    for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)
                    {
                        sum += vxImagePixel(vx_uint8, in, 0, x, y, i, j);
                        count++;
                    }
                }
                sum /= count;
                if (sum > 255)
                    sum = 255;
                vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
            }
        }
    }
}
