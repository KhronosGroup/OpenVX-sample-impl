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

static vx_uint8 u32Tou8(vx_uint32 x)
{
    vx_uint8 ret = 0;
    if (x == 0)
    {
        return 32;
    }
    if ((x & 0x0000FFFF) == 0)
    {
        ret = ret + 16;
        x = x >> 16;
    }
    if ((x & 0x000000FF) == 0)
    {
        ret = ret + 8;
        x = x >> 8;
    }
    if ((x & 0x0000000F) == 0)
    {
        ret = ret + 4;
        x = x >> 4;
    }
    if ((x & 0x00000003) == 0)
    {
        ret = ret + 2;
        x = x >> 2;
    }
    if ((x & 0x00000001) == 0)
    {
        ret = ret + 1;
    }
    return ret;
}

static void s32ShiftR(int32x4_t *pv32x4, vx_int32 shift)
{
    switch(shift)
    {
        case 0:
            break;
        case 1:
            *pv32x4 = vshrq_n_s32(*pv32x4, 1);
            break;
        case 2:
            *pv32x4 = vshrq_n_s32(*pv32x4, 2);
            break;
        case 3:
            *pv32x4 = vshrq_n_s32(*pv32x4, 3);
            break;
        case 4:
            *pv32x4 = vshrq_n_s32(*pv32x4, 4);
            break;
        case 5:
            *pv32x4 = vshrq_n_s32(*pv32x4, 5);
            break;
        case 6:
            *pv32x4 = vshrq_n_s32(*pv32x4, 6);
            break;
        case 7:
            *pv32x4 = vshrq_n_s32(*pv32x4, 7);
            break;
        case 8:
            *pv32x4 = vshrq_n_s32(*pv32x4, 8);
            break;
        case 9:
            *pv32x4 = vshrq_n_s32(*pv32x4, 9);
            break;
        case 10:
            *pv32x4 = vshrq_n_s32(*pv32x4, 10);
            break;
        case 11:
            *pv32x4 = vshrq_n_s32(*pv32x4, 11);
            break;
        case 12:
            *pv32x4 = vshrq_n_s32(*pv32x4, 12);
            break;
        case 13:
            *pv32x4 = vshrq_n_s32(*pv32x4, 13);
            break;
        case 14:
            *pv32x4 = vshrq_n_s32(*pv32x4, 14);
            break;
        case 15:
            *pv32x4 = vshrq_n_s32(*pv32x4, 15);
            break;
        case 16:
            *pv32x4 = vshrq_n_s32(*pv32x4, 16);
            break;
        case 17:
            *pv32x4 = vshrq_n_s32(*pv32x4, 17);
            break;
        case 18:
            *pv32x4 = vshrq_n_s32(*pv32x4, 18);
            break;
        case 19:
            *pv32x4 = vshrq_n_s32(*pv32x4, 19);
            break;
        case 20:
            *pv32x4 = vshrq_n_s32(*pv32x4, 20);
            break;
        case 21:
            *pv32x4 = vshrq_n_s32(*pv32x4, 21);
            break;
        case 22:
            *pv32x4 = vshrq_n_s32(*pv32x4, 22);
            break;
        case 23:
            *pv32x4 = vshrq_n_s32(*pv32x4, 23);
            break;
        case 24:
            *pv32x4 = vshrq_n_s32(*pv32x4, 24);
            break;
        case 25:
            *pv32x4 = vshrq_n_s32(*pv32x4, 25);
            break;
        case 26:
            *pv32x4 = vshrq_n_s32(*pv32x4, 26);
            break;
        case 27:
            *pv32x4 = vshrq_n_s32(*pv32x4, 27);
            break;
        case 28:
            *pv32x4 = vshrq_n_s32(*pv32x4, 28);
            break;
        case 29:
            *pv32x4 = vshrq_n_s32(*pv32x4, 29);
            break;
        case 30:
            *pv32x4 = vshrq_n_s32(*pv32x4, 30);
            break;
        case 31:
            *pv32x4 = vshrq_n_s32(*pv32x4, 31);
            break;
        case 32:
            *pv32x4 = vshrq_n_s32(*pv32x4, 32);
            break;
        default:
            break;
    }
    return;
}

static void convStru8u8(int32x4_t *pvOut0, int32x4_t *pvOut1, int32x4_t *pvOut2, int32x4_t *pvOut3,
                        vx_uint8 *dst, vx_uint8 fillCnt)
{
    int32x4_t out0 = *pvOut0;
    int32x4_t out1 = *pvOut1;
    int32x4_t out2 = *pvOut2;
    int32x4_t out3 = *pvOut3;
    int32x4_t vMaxu8 = vdupq_n_s32(UINT8_MAX);
    int32x4_t vZero = vdupq_n_s32(0);
    uint16x8_t vRetLow, vRetHigh;
    vx_uint8 szTmp[16];

    out0 = vminq_s32(out0, vMaxu8);
    out1 = vminq_s32(out1, vMaxu8);
    out2 = vminq_s32(out2, vMaxu8);
    out3 = vminq_s32(out3, vMaxu8);

    out0 = vmaxq_s32(out0, vZero);
    out1 = vmaxq_s32(out1, vZero);
    out2 = vmaxq_s32(out2, vZero);
    out3 = vmaxq_s32(out3, vZero);

    vRetLow =  vreinterpretq_u16_s16(vcombine_s16(vqmovn_s32(out0), vqmovn_s32(out1)));
    vRetHigh = vreinterpretq_u16_s16(vcombine_s16(vqmovn_s32(out2), vqmovn_s32(out3)));

    if (16 == fillCnt)
    {
        vst1q_u8(dst, vcombine_u8(vqmovn_u16(vRetLow), vqmovn_u16(vRetHigh)));
    }
    else
    {
        vst1q_u8(szTmp, vcombine_u8(vqmovn_u16(vRetLow), vqmovn_u16(vRetHigh)));
        for (vx_uint8 idx = 0; idx < fillCnt; idx++)
        {
            dst[idx] = szTmp[idx];
        }
    }

    return;
}

static void convStru8s16(int32x4_t *pvOut0, int32x4_t *pvOut1, int32x4_t *pvOut2, int32x4_t *pvOut3,
                         vx_int16 *dst, vx_uint8 fillCnt)
{
    int32x4_t out0 = *pvOut0;
    int32x4_t out1 = *pvOut1;
    int32x4_t out2 = *pvOut2;
    int32x4_t out3 = *pvOut3;
    int32x4_t vMaxs16 = vdupq_n_s32(INT16_MAX);
    int32x4_t vMins16 = vdupq_n_s32(INT16_MIN);
    vx_int16 szTmp[16];

    out0 = vminq_s32(out0, vMaxs16);
    out1 = vminq_s32(out1, vMaxs16);
    out2 = vminq_s32(out2, vMaxs16);
    out3 = vminq_s32(out3, vMaxs16);

    out0 = vmaxq_s32(out0, vMins16);
    out1 = vmaxq_s32(out1, vMins16);
    out2 = vmaxq_s32(out2, vMins16);
    out3 = vmaxq_s32(out3, vMins16);

    if (16 == fillCnt)
    {
        vst1q_s16(dst, vcombine_s16(vqmovn_s32(out0), vqmovn_s32(out1)));
        vst1q_s16(dst + 8, vcombine_s16(vqmovn_s32(out2), vqmovn_s32(out3)));
    }
    else
    {
        vst1q_s16(szTmp, vcombine_s16(vqmovn_s32(out0), vqmovn_s32(out1)));
        vst1q_s16(szTmp + 8, vcombine_s16(vqmovn_s32(out2), vqmovn_s32(out3)));
        for (vx_uint8 idx = 0; idx < fillCnt; idx++)
        {
            dst[idx] = szTmp[idx];
        }
    }

    return;
}

static void convStrs16u8(int32x4_t *pvOut0, int32x4_t *pvOut1, vx_uint8 *dst, vx_uint8 fillCnt)
{
    int32x4_t out0 = *pvOut0;
    int32x4_t out1 = *pvOut1;
    int32x4_t vMaxu8 = vdupq_n_s32(UINT8_MAX);
    int32x4_t vZero = vdupq_n_s32(0);
    int16x8_t vRet;
    vx_uint8 szTmp[8];

    out0 = vminq_s32(out0, vMaxu8);
    out1 = vminq_s32(out1, vMaxu8);

    out0 = vmaxq_s32(out0, vZero);
    out1 = vmaxq_s32(out1, vZero);

    vRet =  vcombine_s16(vqmovn_s32(out0), vqmovn_s32(out1));
    if (8 == fillCnt)
    {
        vst1_u8(dst, vqmovn_u16(vreinterpretq_u16_s16(vRet)));
    }
    else
    {
        vst1_u8(szTmp, vqmovn_u16(vreinterpretq_u16_s16(vRet)));
        for (vx_uint8 idx = 0; idx < fillCnt; idx++)
        {
            dst[idx] = szTmp[idx];
        }
    }

    return;
}

static void convStrs16s16(int32x4_t *pvOut0, int32x4_t *pvOut1, vx_int16 *dst, vx_uint8 fillCnt)
{
    int32x4_t out0 = *pvOut0;
    int32x4_t out1 = *pvOut1;
    int32x4_t vMaxs16 = vdupq_n_s32(INT16_MAX);
    int32x4_t vMins16 = vdupq_n_s32(INT16_MIN);
    vx_int16 szTmp[8];

    out0 = vminq_s32(out0, vMaxs16);
    out1 = vminq_s32(out1, vMaxs16);

    out0 = vmaxq_s32(out0, vMins16);
    out1 = vmaxq_s32(out1, vMins16);

    if (8 == fillCnt)
    {
        vst1q_s16(dst, vcombine_s16(vqmovn_s32(out0), vqmovn_s32(out1)));
    }
    else
    {
        vst1q_s16(szTmp, vcombine_s16(vqmovn_s32(out0), vqmovn_s32(out1)));
        for (vx_uint8 idx = 0; idx < fillCnt; idx++)
        {
            dst[idx] = szTmp[idx];
        }
    }
}

static void convRow3x1u8(uint8x16_t *pvPrv, uint8x16_t *pvCur, uint8x16_t *pvNxt, vx_int16 *coeff,
                         int32x4_t *pvOut0, int32x4_t *pvOut1, int32x4_t *pvOut2, int32x4_t *pvOut3)
{
    uint8x16_t vPrv = *pvPrv;
    uint8x16_t vCur = *pvCur;
    uint8x16_t vNxt = *pvNxt;

    uint8x16_t vData = vextq_u8(vPrv, vCur, 15);
    int16x8_t s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    int16x8_t s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));

    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[2]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[2]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[2]);

    vData = vextq_u8(vCur, vNxt, 1);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[0]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[0]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[0]);

    vData = vCur;
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[1]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[1]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[1]);

    return;
}

static void convRow5x1u8(uint8x16_t *pvPrv, uint8x16_t *pvCur, uint8x16_t *pvNxt, vx_int16 *coeff,
                         int32x4_t *pvOut0, int32x4_t *pvOut1, int32x4_t *pvOut2, int32x4_t *pvOut3)
{
    uint8x16_t vPrv = *pvPrv;
    uint8x16_t vCur = *pvCur;
    uint8x16_t vNxt = *pvNxt;

    uint8x16_t vData = vextq_u8(vPrv, vCur, 14);
    int16x8_t s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    int16x8_t s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));

    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[4]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[4]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[4]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[4]);

    vData = vextq_u8(vCur, vNxt, 2);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[0]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[0]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[0]);

    vData = vextq_u8(vPrv, vCur, 15);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[3]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[3]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[3]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[3]);

    vData = vextq_u8(vCur, vNxt, 1);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[1]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[1]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[1]);

    vData = vCur;
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[2]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[2]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[2]);

    return;
}

static void convRow7x1u8(uint8x16_t *pvPrv, uint8x16_t *pvCur, uint8x16_t *pvNxt, vx_int16 *coeff,
                         int32x4_t *pvOut0, int32x4_t *pvOut1, int32x4_t *pvOut2, int32x4_t *pvOut3)
{
    uint8x16_t vPrv = *pvPrv;
    uint8x16_t vCur = *pvCur;
    uint8x16_t vNxt = *pvNxt;

    uint8x16_t vData = vextq_u8(vPrv, vCur, 13);
    int16x8_t s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    int16x8_t s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));

    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[6]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[6]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[6]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[6]);

    vData = vextq_u8(vCur, vNxt, 3);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[0]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[0]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[0]);

    vData = vextq_u8(vPrv, vCur, 14);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));

    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[5]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[5]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[5]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[5]);

    vData = vextq_u8(vCur, vNxt, 2);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[1]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[1]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[1]);

    vData = vextq_u8(vPrv, vCur, 15);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[4]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[4]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[4]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[4]);

    vData = vextq_u8(vCur, vNxt, 1);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[2]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[2]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[2]);

    vData = vCur;
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[3]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[3]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[3]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[3]);

    return;
}

static void convRow9x1u8(uint8x16_t *pvPrv, uint8x16_t *pvCur, uint8x16_t *pvNxt, vx_int16 *coeff,
                         int32x4_t *pvOut0, int32x4_t *pvOut1, int32x4_t *pvOut2, int32x4_t *pvOut3)
{
    uint8x16_t vPrv = *pvPrv;
    uint8x16_t vCur = *pvCur;
    uint8x16_t vNxt = *pvNxt;

    uint8x16_t vData = vextq_u8(vPrv, vCur, 12);
    int16x8_t s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    int16x8_t s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));

    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[8]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[8]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[8]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[8]);

    vData = vextq_u8(vCur, vNxt, 4);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[0]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[0]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[0]);

    vData = vextq_u8(vPrv, vCur, 13);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[7]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[7]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[7]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[7]);

    vData = vextq_u8(vCur, vNxt, 3);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[1]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[1]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[1]);

    vData = vextq_u8(vPrv, vCur, 14);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[6]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[6]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[6]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[6]);

    vData = vextq_u8(vCur, vNxt, 2);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[2]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[2]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[2]);

    vData = vextq_u8(vPrv, vCur, 15);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[5]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[5]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[5]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[5]);

    vData = vextq_u8(vCur, vNxt, 1);
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[3]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[3]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[3]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[3]);

    vData = vCur;
    s16Tmp0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(vData)));
    s16Tmp1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(vData)));
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(s16Tmp0), coeff[4]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(s16Tmp0), coeff[4]);
    *pvOut2 = vmlal_n_s16(*pvOut2, vget_low_s16(s16Tmp1), coeff[4]);
    *pvOut3 = vmlal_n_s16(*pvOut3, vget_high_s16(s16Tmp1), coeff[4]);

    return;
}


static void convRow3x1s16(int16x8_t *pvPrv, int16x8_t *pvCur, int16x8_t *pvNxt, vx_int16 *coeff,
                          int32x4_t *pvOut0, int32x4_t *pvOut1)
{
    int16x8_t vPrv = *pvPrv;
    int16x8_t vCur = *pvCur;
    int16x8_t vNxt = *pvNxt;

    int16x8_t vData = vextq_s16(vPrv, vCur, 7);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[2]);

    vData = vextq_s16(vCur, vNxt, 1);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[0]);

    vData = vCur;
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[1]);

    return;
}

static void convRow5x1s16(int16x8_t *pvPrv, int16x8_t *pvCur, int16x8_t *pvNxt, vx_int16 *coeff,
                          int32x4_t *pvOut0, int32x4_t *pvOut1)
{
    int16x8_t vPrv = *pvPrv;
    int16x8_t vCur = *pvCur;
    int16x8_t vNxt = *pvNxt;

    int16x8_t vData = vextq_s16(vPrv, vCur, 6);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[4]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[4]);

    vData = vextq_s16(vCur, vNxt, 2);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[0]);

    vData = vextq_s16(vPrv, vCur, 7);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[3]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[3]);

    vData = vextq_s16(vCur, vNxt, 1);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[1]);

    vData = vCur;
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[2]);

    return;
}

static void convRow7x1s16(int16x8_t *pvPrv, int16x8_t *pvCur, int16x8_t *pvNxt, vx_int16 *coeff,
                          int32x4_t *pvOut0, int32x4_t *pvOut1)
{
    int16x8_t vPrv = *pvPrv;
    int16x8_t vCur = *pvCur;
    int16x8_t vNxt = *pvNxt;

    int16x8_t vData = vextq_s16(vPrv, vCur, 5);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[6]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[6]);

    vData = vextq_s16(vCur, vNxt, 3);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[0]);


    vData = vextq_s16(vPrv, vCur, 6);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[5]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[5]);

    vData = vextq_s16(vCur, vNxt, 2);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[1]);

    vData = vextq_s16(vPrv, vCur, 7);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[4]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[4]);

    vData = vextq_s16(vCur, vNxt, 1);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[2]);

    vData = vCur;
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[3]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[3]);

    return;
}

static void convRow9x1s16(int16x8_t *pvPrv, int16x8_t *pvCur, int16x8_t *pvNxt, vx_int16 *coeff,
                          int32x4_t *pvOut0, int32x4_t *pvOut1)
{
    int16x8_t vPrv = *pvPrv;
    int16x8_t vCur = *pvCur;
    int16x8_t vNxt = *pvNxt;

    int16x8_t vData = vextq_s16(vPrv, vCur, 4);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[8]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[8]);

    vData = vextq_s16(vCur, vNxt, 4);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[0]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[0]);

    vData = vextq_s16(vPrv, vCur, 5);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[7]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[7]);

    vData = vextq_s16(vCur, vNxt, 3);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[1]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[1]);


    vData = vextq_s16(vPrv, vCur, 6);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[6]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[6]);

    vData = vextq_s16(vCur, vNxt, 2);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[2]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[2]);

    vData = vextq_s16(vPrv, vCur, 7);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[5]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[5]);

    vData = vextq_s16(vCur, vNxt, 1);
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[3]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[3]);

    vData = vCur;
    *pvOut0 = vmlal_n_s16(*pvOut0, vget_low_s16(vData), coeff[4]);
    *pvOut1 = vmlal_n_s16(*pvOut1, vget_high_s16(vData), coeff[4]);

    return;
}

void Convolve_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_convolution_t *conv = (vx_tile_convolution_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = out->tile_x + out->tile_block.width;

    vx_size conv_width = (*conv).conv_width;
    vx_size conv_height = (*conv).conv_height;

    vx_int32 conv_radius_x, conv_radius_y;

    conv_radius_x = (vx_int32)conv_width / 2;
    conv_radius_y = (vx_int32)conv_height / 2;

    vx_uint32 src_format = in->image.format;
    vx_uint32 dst_format = out->image.format;

    vx_int32 sum = 0, value = 0;

    vx_uint32 scale = (*conv).scale;

    vx_int16 conv_mat[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };

    memcpy(conv_mat, ((*conv).conv_mat), conv_width * conv_height * sizeof(vx_int16));

    vx_int32 shift = (vx_int32)u32Tou8(scale);

    vx_border_t bordermode = in->border;

    vx_uint32 w16 = (vxTileWidth(out, 0) >> 4) << 4;
    vx_uint32 w8 = (vxTileWidth(out, 0) >> 3) << 3;

    if ( high_y == vxTileHeight(out, 0) && high_x == vxTileWidth(out, 0) )
    {
        if (bordermode.mode == VX_BORDER_UNDEFINED)
        {
            if (src_format == VX_DF_IMAGE_U8)
            {
                uint8x16_t vPrv[C_MAX_CONVOLUTION_DIM];
                uint8x16_t vCur[C_MAX_CONVOLUTION_DIM];
                uint8x16_t vNxt[C_MAX_CONVOLUTION_DIM];
                int32x4_t out0 = vdupq_n_s32(0);
                int32x4_t out1 = out0;
                int32x4_t out2 = out0;
                int32x4_t out3 = out0;

                vx_uint32 dstY = conv_radius_y;
                vx_uint8 *dstTmp;
                for (x = 0; x < w16; x+=16)
                {
                    dstTmp = (vx_uint8 *)dst_base + x * out->addr->stride_x;
                    dstY = conv_radius_y;
                    if (0 == x)
                    {
                        for (y = 0; y < conv_height; y++)
                        {
                            vPrv[y] = vdupq_n_u8(0);
                            vCur[y] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x);
                            vNxt[y] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                        }
                    }
                    else
                    {
                        for (y = 0; y < conv_height; y++)
                        {
                            vPrv[y] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x - 16) * in->addr->stride_x);
                            vCur[y] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x);
                            vNxt[y] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                        }
                    }

                    for (y = conv_height; y < high_y; (++y, dstY++))
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        out2 = out0;
                        out3 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);
                        s32ShiftR(&out2, shift);
                        s32ShiftR(&out3, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * out->addr->stride_y, 16);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + dstY * out->addr->stride_y), 16);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        if (0 == x)
                        {
                            vPrv[conv_height - 1] = vdupq_n_u8(0);
                        }
                        else
                        {
                            vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x - 16) * in->addr->stride_x);
                        }
                        vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x);
                        vNxt[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                    }

                    //process the last one
                    out0 = vdupq_n_s32(0);
                    out1 = out0;
                    out2 = out0;
                    out3 = out0;
                    for (vx_uint8 convY = 0; convY < conv_height; convY++)
                    {
                        if (3 == conv_width)
                        {
                            convRow3x1u8(&(vPrv[convY]), &(vCur[convY]), &(vNxt[convY]),
                                         conv_mat + (conv_height - (convY + 1)) * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                        else if (5 == conv_width)
                        {
                            convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                         conv_mat + (conv_height - (convY + 1))  * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                        else if (7 == conv_width)
                        {
                            convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                         conv_mat + (conv_height - (convY + 1))  * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                        else if (9 == conv_width)
                        {
                            convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                         conv_mat + (conv_height - (convY + 1))  * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                    }

                    s32ShiftR(&out0, shift);
                    s32ShiftR(&out1, shift);
                    s32ShiftR(&out2, shift);
                    s32ShiftR(&out3, shift);

                    if (dst_format == VX_DF_IMAGE_U8)
                    {
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * out->addr->stride_y, 16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + dstY * out->addr->stride_y), 16);
                    }
                }

                //process left data
                if (w16 < vxTileWidth(out, 0))
                {
                    dstTmp = (vx_uint8 *)dst_base + w16 * out->addr->stride_x;
                    dstY = conv_radius_y;

                    for (y = 0; y < conv_height; y++)
                    {
                        vPrv[y] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (w16 - 16) * in->addr->stride_x);
                        vCur[y] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + w16 * in->addr->stride_x);
                        vNxt[y] = vdupq_n_u8(0);
                    }

                    for (y = conv_height; y < high_y; (++y, dstY++))
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        out2 = out0;
                        out3 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);
                        s32ShiftR(&out2, shift);
                        s32ShiftR(&out3, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * out->addr->stride_y,
                                        vxTileWidth(out, 0) - w16);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStru8s16(&out0, &out1, &out2, &out3,
                                         (vx_int16 *)(dstTmp + dstY * out->addr->stride_y),
                                         vxTileWidth(out, 0) - w16);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (w16 - 16) * in->addr->stride_x);
                        if ((y + 1) != high_y)
                        {
                            vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + w16 * in->addr->stride_x);
                        }
                        else
                        {
                            vx_uint8 szTmp[16];
                            memcpy(szTmp, (vx_uint8 *)src_base + y * in->addr->stride_y + w16 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w16) * in->addr->stride_x);
                            vCur[conv_height - 1] = vld1q_u8(szTmp);
                        }
                    }

                    //process the last one
                    out0 = vdupq_n_s32(0);
                    out1 = out0;
                    out2 = out0;
                    out3 = out0;
                    for (vx_uint8 convY = 0; convY < conv_height; convY++)
                    {
                        if (3 == conv_width)
                        {
                            convRow3x1u8(&(vPrv[convY]), &(vCur[convY]), &(vNxt[convY]),
                                         conv_mat + (conv_height - (convY + 1)) * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                        else if (5 == conv_width)
                        {
                            convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                         conv_mat + (conv_height - (convY + 1))  * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                        else if (7 == conv_width)
                        {
                            convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                         conv_mat + (conv_height - (convY + 1))  * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                        else if (9 == conv_width)
                        {
                            convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                         conv_mat + (conv_height - (convY + 1))  * conv_width,
                                         &out0, &out1, &out2, &out3);
                        }
                    }

                    s32ShiftR(&out0, shift);
                    s32ShiftR(&out1, shift);
                    s32ShiftR(&out2, shift);
                    s32ShiftR(&out3, shift);

                    if (dst_format == VX_DF_IMAGE_U8)
                    {
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * out->addr->stride_y,
                                    vxTileWidth(out, 0) - w16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3,
                                     (vx_int16 *)(dstTmp + dstY * out->addr->stride_y),
                                     vxTileWidth(out, 0) - w16);
                    }
                }
            }
            else if (src_format == VX_DF_IMAGE_S16)
            {
                int16x8_t vPrv[C_MAX_CONVOLUTION_DIM];
                int16x8_t vCur[C_MAX_CONVOLUTION_DIM];
                int16x8_t vNxt[C_MAX_CONVOLUTION_DIM];
                int32x4_t out0 = vdupq_n_s32(0);
                int32x4_t out1 = out0;
                vx_uint32 w8 = (vxTileWidth(out, 0) >> 3) << 3;
                vx_uint32 dstY = conv_radius_y;
                vx_uint8 *dstTmp;
                for (x = 0; x < w8; x+=8)
                {
                    dstTmp = (vx_uint8 *)dst_base + x * out->addr->stride_x;
                    dstY = conv_radius_y;
                    if (0 == x)
                    {
                        for (y = 0; y < conv_height; y++)
                        {
                            vPrv[y] = vdupq_n_s16(0);
                            vCur[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x));
                            vNxt[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                        }
                    }
                    else
                    {
                        for (y = 0; y < conv_height; y++)
                        {
                            vPrv[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x - 8) * in->addr->stride_x));
                            vCur[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x));
                            vNxt[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                        }
                    }

                    for (y = conv_height; y < high_y; (++y, dstY++))
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStrs16u8(&out0, &out1, dstTmp + dstY * out->addr->stride_y, 8);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * out->addr->stride_y), 8);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        if (0 == x)
                        {
                            vPrv[conv_height - 1] = vdupq_n_s16(0);
                        }
                        else
                        {
                            vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x - 8) * in->addr->stride_x));
                        }
                        vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x));
                        vNxt[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                    }

                    //process the last one
                    out0 = vdupq_n_s32(0);
                    out1 = out0;
                    for (vx_uint8 convY = 0; convY < conv_height; convY++)
                    {
                        if (3 == conv_width)
                        {
                            convRow3x1s16(&(vPrv[convY]), &(vCur[convY]), &(vNxt[convY]),
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                        else if (5 == conv_width)
                        {
                            convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                        else if (7 == conv_width)
                        {
                            convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                        else if (9 == conv_width)
                        {
                            convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                    }

                    s32ShiftR(&out0, shift);
                    s32ShiftR(&out1, shift);

                    if (dst_format == VX_DF_IMAGE_U8)
                    {
                        convStrs16u8(&out0, &out1, dstTmp + dstY * out->addr->stride_y, 8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * out->addr->stride_y), 8);
                    }
                }

                if (w8 < vxTileWidth(out, 0))
                {
                    dstTmp = (vx_uint8 *)dst_base + w8 * out->addr->stride_x;
                    dstY = conv_radius_y;

                    for (y = 0; y < conv_height; y++)
                    {
                        vPrv[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (w8 - 8) * in->addr->stride_x));
                        vCur[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + w8 * in->addr->stride_x));
                        vNxt[y] = vdupq_n_s16(0);
                    }

                    for (y = conv_height; y < high_y; (++y, dstY++))
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStrs16u8(&out0, &out1, dstTmp + dstY * out->addr->stride_y, vxTileWidth(out, 0) - w8);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * out->addr->stride_y), vxTileWidth(out, 0) - w8);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (w8 - 8) * in->addr->stride_x));
                        if ((y + 1) != high_y)
                        {
                            vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + w8 * in->addr->stride_x));
                        }
                        else
                        {
                            vx_int16 szTmp[8];
                            memcpy(szTmp, (vx_uint8 *)src_base + y * in->addr->stride_y + w8 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w8) * in->addr->stride_x);
                            vCur[conv_height - 1] = vld1q_s16(szTmp);
                        }
                    }

                    //process the last one
                    out0 = vdupq_n_s32(0);
                    out1 = out0;
                    for (vx_uint8 convY = 0; convY < conv_height; convY++)
                    {
                        if (3 == conv_width)
                        {
                            convRow3x1s16(&(vPrv[convY]), &(vCur[convY]), &(vNxt[convY]),
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                        else if (5 == conv_width)
                        {
                            convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                        else if (7 == conv_width)
                        {
                            convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                        else if (9 == conv_width)
                        {
                            convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                          conv_mat + (conv_height - (convY + 1)) * conv_width,
                                          &out0, &out1);
                        }
                    }

                    s32ShiftR(&out0, shift);
                    s32ShiftR(&out1, shift);

                    if (dst_format == VX_DF_IMAGE_U8)
                    {
                        convStrs16u8(&out0, &out1, dstTmp + dstY * out->addr->stride_y, vxTileWidth(out, 0) - w8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * out->addr->stride_y),
                                      vxTileWidth(out, 0) - w8);
                    }
                }
            }
        }
        else if (bordermode.mode == VX_BORDER_REPLICATE)
        {
            if (src_format == VX_DF_IMAGE_U8)
            {
                uint8x16_t vPrv[C_MAX_CONVOLUTION_DIM];
                uint8x16_t vCur[C_MAX_CONVOLUTION_DIM];
                uint8x16_t vNxt[C_MAX_CONVOLUTION_DIM];
                int32x4_t out0 = vdupq_n_s32(0);
                int32x4_t out1 = out0;
                int32x4_t out2 = out0;
                int32x4_t out3 = out0;
                vx_uint32 nextY;
                vx_int32 tmpY;
                vx_uint32 idx;
                vx_uint8 *dstTmp;
                vx_uint8 szTmpData[16];
                for (x = 0; x < w16; x+=16)
                {
                    dstTmp = (vx_uint8 *)dst_base + x * out->addr->stride_x;
                    if (0 == x)
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            if (y < 0)
                            {
                                vPrv[idx] = vdupq_n_u8(*(vx_uint8 *)src_base);
                                vCur[idx] = vld1q_u8((vx_uint8 *)src_base);
                                if ((x + 16) >= w16)
                                {
                                    memcpy(szTmpData, (vx_uint8 *)src_base + (x + 16) * in->addr->stride_x,
                                           (vxTileWidth(out, 0) - (x + 16)) * sizeof(vx_uint8));
                                    memset(&szTmpData[vxTileWidth(out, 0) - (x + 16)],
                                           *((vx_uint8 *)src_base + vxTileWidth(out, 0) - 1),
                                           16 - (vxTileWidth(out, 0) - (x + 16)));
                                    vNxt[idx] = vld1q_u8(szTmpData);
                                }
                                else
                                {
                                    vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + (x + 16) * in->addr->stride_x);
                                }
                            }
                            else
                            {
                                vPrv[idx] = vdupq_n_u8(*((vx_uint8 *)src_base + y * in->addr->stride_y));
                                vCur[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y);
                                if ((x + 16) >= w16)
                                {
                                    memcpy(szTmpData, (vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x,
                                           (vxTileWidth(out, 0) - (x + 16)) * sizeof(vx_uint8));
                                    memset(&szTmpData[vxTileWidth(out, 0) - (x + 16)],
                                           *((vx_uint8 *)src_base + y * in->addr->stride_y + vxTileWidth(out, 0) - 1),
                                           16 - (vxTileWidth(out, 0) - (x + 16)));
                                    vNxt[idx] = vld1q_u8(szTmpData);
                                }
                                else
                                {
                                    vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                                }
                            }
                        }
                    }
                    else
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            tmpY = y;
                            if (y < 0)
                            {
                                tmpY = 0;
                            }
                            vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + tmpY * in->addr->stride_y + (x - 16) * in->addr->stride_x);
                            vCur[idx] = vld1q_u8((vx_uint8 *)src_base + tmpY * in->addr->stride_y + x * in->addr->stride_x);
                            if ((x + 16) >= w16)
                            {
                                memcpy(szTmpData, (vx_uint8 *)src_base + tmpY * in->addr->stride_y + (x + 16) * in->addr->stride_x,
                                       vxTileWidth(out, 0) - (x + 16));
                                for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 16); tmpIdx < 16; tmpIdx++)
                                {
                                    szTmpData[tmpIdx] = *((vx_uint8 *)src_base + tmpY * in->addr->stride_y + vxTileWidth(out, 0) - 1);
                                }
                                vNxt[idx] = vld1q_u8(szTmpData);
                            }
                            else
                            {
                                vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + tmpY * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                            }
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        out2 = out0;
                        out3 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);
                        s32ShiftR(&out2, shift);
                        s32ShiftR(&out3, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * out->addr->stride_y, 16);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + y * out->addr->stride_y), 16);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            nextY = high_y - 1;
                        }

                        if (0 == x)
                        {
                            vPrv[conv_height - 1] = vdupq_n_u8(*((vx_uint8 *)src_base + nextY * in->addr->stride_y));
                        }
                        else
                        {
                            vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x - 16) * in->addr->stride_x);
                        }
                        vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + x * in->addr->stride_x);
                        if ((x + 16) >= w16)
                        {
                            memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 16) * in->addr->stride_x,
                                   vxTileWidth(out, 0) - (x + 16));
                            for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 16); tmpIdx < 16; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = *((vx_uint8 *)src_base + nextY * in->addr->stride_y + vxTileWidth(out, 0) - 1);
                            }
                            vNxt[conv_height - 1] = vld1q_u8(szTmpData);
                        }
                        else
                        {
                            vNxt[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                        }
                    }
                }

                //process left data
                if (w16 < vxTileWidth(out, 0))
                {
                    dstTmp = (vx_uint8 *)dst_base + w16 * out->addr->stride_x;

                    for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                    {
                        if (y < 0)
                        {
                            vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + (w16 - 16) * in->addr->stride_x);
                            memcpy(szTmpData, (vx_uint8 *)src_base + w16 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w16) * sizeof(vx_uint8));
                            memset(&szTmpData[vxTileWidth(out, 0) - w16], *((vx_uint8 *)src_base + vxTileWidth(out, 0) - 1),
                                   16 - (vxTileWidth(out, 0) - w16));
                            vCur[idx] = vld1q_u8(szTmpData);
                            vNxt[idx] = vdupq_n_u8(szTmpData[15]);
                        }
                        else
                        {
                            vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (w16 - 16) * in->addr->stride_x);
                            memcpy(szTmpData, (vx_uint8 *)src_base + y * in->addr->stride_y + w16 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w16) * sizeof(vx_uint8));
                            memset(&szTmpData[vxTileWidth(out, 0) - w16], *((vx_uint8 *)src_base + y * in->addr->stride_y + vxTileWidth(out, 0) - 1),
                                   16 - (vxTileWidth(out, 0) - w16));
                            vCur[idx] = vld1q_u8(szTmpData);
                            vNxt[idx] = vdupq_n_u8(szTmpData[15]);
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        out2 = out0;
                        out3 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);
                        s32ShiftR(&out2, shift);
                        s32ShiftR(&out3, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * out->addr->stride_y,
                                        vxTileWidth(out, 0) - w16);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStru8s16(&out0, &out1, &out2, &out3,
                                         (vx_int16 *)(dstTmp + y * out->addr->stride_y),
                                         vxTileWidth(out, 0) - w16);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            nextY = high_y - 1;
                        }

                        vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + (w16 - 16) * in->addr->stride_x);
                        memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + w16 * in->addr->stride_x,
                               (vxTileWidth(out, 0) - w16) * sizeof(vx_uint8));
                        memset(&szTmpData[vxTileWidth(out, 0) - w16], *((vx_uint8 *)src_base + nextY * in->addr->stride_y + vxTileWidth(out, 0) - 1),
                               16 - (vxTileWidth(out, 0) - w16));
                        vCur[conv_height - 1] = vld1q_u8(szTmpData);
                        vNxt[conv_height - 1] = vdupq_n_u8(szTmpData[15]);
                    }
                }
            }
            else if (src_format == VX_DF_IMAGE_S16)
            {
                int16x8_t vPrv[C_MAX_CONVOLUTION_DIM];
                int16x8_t vCur[C_MAX_CONVOLUTION_DIM];
                int16x8_t vNxt[C_MAX_CONVOLUTION_DIM];
                int32x4_t out0 = vdupq_n_s32(0);
                int32x4_t out1 = out0;
                vx_uint32 idx;
                vx_uint32 nextY;
                vx_int32 tmpY;
                vx_uint8 *dstTmp;
                vx_int16 szTmpData[8];
                for (x = 0; x < w8; x+=8)
                {
                    dstTmp = (vx_uint8 *)dst_base + x * out->addr->stride_x;
                    if (0 == x)
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            if (y < 0)
                            {
                                vPrv[idx] = vdupq_n_s16(*(vx_int16 *)src_base);
                                vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + x * in->addr->stride_x));
                                vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + (x + 8) * in->addr->stride_x));
                            }
                            else
                            {
                                vPrv[idx] = vdupq_n_s16(*(vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y));
                                vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x));
                                vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                            }
                        }
                    }
                    else
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            tmpY = y;
                            if (y < 0)
                            {
                                tmpY = 0;
                            }
                            vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + tmpY * in->addr->stride_y + (x - 8) * in->addr->stride_x));
                            vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + tmpY * in->addr->stride_y + x * in->addr->stride_x));
                            if ((x + 8) >= w8)
                            {
                                memcpy(szTmpData, (vx_uint8 *)src_base + tmpY * in->addr->stride_y + (x + 8) * in->addr->stride_x,
                                       (vxTileWidth(out, 0) - (x + 8)) * sizeof(vx_int16));
                                for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 8); tmpIdx < 8; tmpIdx++)
                                {
                                    szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + tmpY * in->addr->stride_y + vxTileWidth(out, 0) - 2);
                                }
                                vNxt[idx] = vld1q_s16(szTmpData);
                            }
                            else
                            {
                                vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + tmpY * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                            }
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStrs16u8(&out0, &out1, dstTmp + y * out->addr->stride_y, 8);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * out->addr->stride_y), 8);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            nextY = high_y - 1;
                        }

                        if (0 == x)
                        {
                            vPrv[conv_height - 1] = vdupq_n_s16(*(vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y));
                        }
                        else
                        {
                            vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x - 8) * in->addr->stride_x));
                        }
                        vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + x * in->addr->stride_x));
                        if ((x + 8) >= w8)
                        {
                            memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 8) * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - (x + 8)) * sizeof(vx_int16));
                            for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 8); tmpIdx < 8; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + vxTileWidth(out, 0) - 2);
                            }
                            vNxt[conv_height - 1] = vld1q_s16(szTmpData);
                        }
                        else
                        {
                            vNxt[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                        }
                    }
                }

                if (w8 < vxTileWidth(out, 0))
                {
                    dstTmp = (vx_uint8 *)dst_base + w8 * out->addr->stride_x;

                    for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                    {
                        if (y < 0)
                        {
                            vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + (w8 - 8) * in->addr->stride_x));
                            memcpy(szTmpData, (vx_uint8 *)src_base + w8 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w8) * sizeof(vx_int16));
                            for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - w8; tmpIdx < 8; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + vxTileWidth(out, 0) - 2);
                            }
                            vCur[idx] = vld1q_s16(szTmpData);
                            vNxt[idx] = vdupq_n_s16(szTmpData[7]);
                        }
                        else
                        {
                            vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (w8 - 8) * in->addr->stride_x));
                            memcpy(szTmpData, (vx_uint8 *)src_base + y * in->addr->stride_y + w8 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w8) * sizeof(vx_int16));
                            for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - w8; tmpIdx < 8; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + vxTileWidth(out, 0) - 2);
                            }
                            vCur[idx] = vld1q_s16(szTmpData);
                            vNxt[idx] = vdupq_n_s16(szTmpData[7]);
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStrs16u8(&out0, &out1, dstTmp + y * out->addr->stride_y, vxTileWidth(out, 0) - w8);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * out->addr->stride_y), vxTileWidth(out, 0) - w8);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            nextY = high_y - 1;
                        }

                        vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + (w8 - 8) * in->addr->stride_x));
                        memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + w8 * in->addr->stride_x,
                               (vxTileWidth(out, 0) - w8) * sizeof(vx_int16));
                        for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - w8; tmpIdx < 8; tmpIdx++)
                        {
                            szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + vxTileWidth(out, 0) - 2);
                        }
                        vCur[conv_height - 1] = vld1q_s16(szTmpData);
                        vNxt[conv_height - 1] = vdupq_n_s16(szTmpData[7]);
                    }
                }
            }
        }
        else if (bordermode.mode == VX_BORDER_CONSTANT)
        {
            if (src_format == VX_DF_IMAGE_U8)
            {
                uint8x16_t vPrv[C_MAX_CONVOLUTION_DIM];
                uint8x16_t vCur[C_MAX_CONVOLUTION_DIM];
                uint8x16_t vNxt[C_MAX_CONVOLUTION_DIM];
                int32x4_t out0 = vdupq_n_s32(0);
                int32x4_t out1 = out0;
                int32x4_t out2 = out0;
                int32x4_t out3 = out0;
                vx_pixel_value_t cval = bordermode.constant_value;
                vx_uint32 nextY;
                vx_uint32 idx;
                vx_uint8 *dstTmp;
                vx_uint8 szTmpData[16];
                for (x = 0; x < w16; x+=16)
                {
                    dstTmp = (vx_uint8 *)dst_base + x * out->addr->stride_x;
                    if (0 == x)
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            if (y < 0)
                            {
                                vPrv[idx] = vdupq_n_u8(cval.U8);
                                vCur[idx] = vPrv[idx];
                                vNxt[idx] = vCur[idx];
                            }
                            else
                            {
                                vPrv[idx] = vdupq_n_u8(cval.U8);
                                vCur[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y);
                                vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                            }
                        }
                    }
                    else
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            if (y < 0)
                            {
                                vPrv[idx] = vdupq_n_u8(cval.U8);
                                vCur[idx] = vPrv[idx];
                                vNxt[idx] = vCur[idx];
                            }
                            else
                            {
                                vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x - 16) * in->addr->stride_x);
                                vCur[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x);
                                if ((x + 16) >= w16)
                                {
                                    memcpy(szTmpData, (vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x,
                                           vxTileWidth(out, 0) - (x + 16));
                                    for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 16); tmpIdx < 16; tmpIdx++)
                                    {
                                        szTmpData[tmpIdx] = cval.U8;
                                    }
                                    vNxt[idx] = vld1q_u8(szTmpData);
                                }
                                else
                                {
                                    vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                                }
                            }
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        out2 = out0;
                        out3 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);
                        s32ShiftR(&out2, shift);
                        s32ShiftR(&out3, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * out->addr->stride_y, 16);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + y * out->addr->stride_y), 16);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            vPrv[conv_height - 1] = vdupq_n_u8(cval.U8);
                            vCur[conv_height - 1] = vPrv[conv_height - 1];
                            vNxt[conv_height - 1] = vPrv[conv_height - 1];
                        }
                        else
                        {
                            if (0 == x)
                            {
                                vPrv[conv_height - 1] = vdupq_n_u8(cval.U8);
                            }
                            else
                            {
                                vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x - 16) * in->addr->stride_x);
                            }
                            vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + x * in->addr->stride_x);
                            if ((x + 16) >= w16)
                            {
                                memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 16) * in->addr->stride_x,
                                       vxTileWidth(out, 0) - (x + 16));
                                for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 16); tmpIdx < 16; tmpIdx++)
                                {
                                    szTmpData[tmpIdx] = cval.U8;
                                }
                                vNxt[conv_height - 1] = vld1q_u8(szTmpData);
                            }
                            else
                            {
                                vNxt[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 16) * in->addr->stride_x);
                            }
                        }
                    }
                }

                //process left data
                if (w16 < vxTileWidth(out, 0))
                {
                    dstTmp = (vx_uint8 *)dst_base + w16 * out->addr->stride_x;

                    for (y = -conv_radius_y, idx = 0; y <= conv_height; (y++, idx++))
                    {
                        if (y < 0)
                        {
                            vPrv[idx] = vdupq_n_u8(cval.U8);
                            vCur[idx] = vPrv[idx];
                            vNxt[idx] = vPrv[idx];
                        }
                        else
                        {
                            vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + y * in->addr->stride_y + (w16 - 16) * in->addr->stride_x);
                            memcpy(szTmpData, (vx_uint8 *)src_base + y * in->addr->stride_y + w16 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w16) * sizeof(vx_uint8));
                            memset(&szTmpData[vxTileWidth(out, 0) - w16], cval.U8,
                                   16 - (vxTileWidth(out, 0) - w16));
                            vCur[idx] = vld1q_u8(szTmpData);
                            vNxt[idx] = vdupq_n_u8(cval.U8);
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        out2 = out0;
                        out3 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1u8(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                             conv_mat + (conv_height - (convY + 1))  * conv_width,
                                             &out0, &out1, &out2, &out3);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);
                        s32ShiftR(&out2, shift);
                        s32ShiftR(&out3, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * out->addr->stride_y,
                                        vxTileWidth(out, 0) - w16);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStru8s16(&out0, &out1, &out2, &out3,
                                         (vx_int16 *)(dstTmp + y * out->addr->stride_y),
                                         vxTileWidth(out, 0) - w16);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            vPrv[conv_height - 1] = vdupq_n_u8(cval.U8);
                            vCur[conv_height - 1] = vPrv[conv_height - 1];
                            vNxt[conv_height - 1] = vPrv[conv_height - 1];
                        }
                        else
                        {
                            vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * in->addr->stride_y + (w16 - 16) * in->addr->stride_x);
                            memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + w16 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w16) * sizeof(vx_uint8));
                            memset(&szTmpData[vxTileWidth(out, 0) - w16], cval.U8,
                                   16 - (vxTileWidth(out, 0) - w16));
                            vCur[conv_height - 1] = vld1q_u8(szTmpData);
                            vNxt[conv_height - 1] = vdupq_n_u8(cval.U8);
                        }
                    }
                }
            }
            else if (src_format == VX_DF_IMAGE_S16)
            {
                int16x8_t vPrv[C_MAX_CONVOLUTION_DIM];
                int16x8_t vCur[C_MAX_CONVOLUTION_DIM];
                int16x8_t vNxt[C_MAX_CONVOLUTION_DIM];
                int32x4_t out0 = vdupq_n_s32(0);
                int32x4_t out1 = out0;
                vx_pixel_value_t cval = bordermode.constant_value;
                vx_uint32 idx;
                vx_uint32 nextY;
                vx_uint8 *dstTmp;
                vx_int16 szTmpData[8];
                for (x = 0; x < w8; x+=8)
                {
                    dstTmp = (vx_uint8 *)dst_base + x * out->addr->stride_x;
                    if (0 == x)
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            if (y < 0)
                            {
                                vPrv[idx] = vdupq_n_s16(cval.S16);
                                vCur[idx] = vPrv[idx];
                                vNxt[idx] = vPrv[idx];
                            }
                            else
                            {
                                vPrv[idx] = vdupq_n_s16(cval.S16);
                                vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x));
                                vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                            }
                        }
                    }
                    else
                    {
                        for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                        {
                            if (y < 0)
                            {
                                vPrv[idx] = vdupq_n_s16(cval.S16);
                                vCur[idx] = vPrv[idx];
                                vNxt[idx] = vPrv[idx];
                            }
                            else
                            {
                                vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x - 8) * in->addr->stride_x));
                                vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + x * in->addr->stride_x));
                                if ((x + 8) >= w8)
                                {
                                    memcpy(szTmpData, (vx_uint8 *)src_base + y * in->addr->stride_y + (x + 8) * in->addr->stride_x,
                                           (vxTileWidth(out, 0) - (x + 8)) * sizeof(vx_int16));
                                    for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 8); tmpIdx < 8; tmpIdx++)
                                    {
                                        szTmpData[tmpIdx] = cval.S16;
                                    }
                                    vNxt[idx] = vld1q_s16(szTmpData);
                                }
                                else
                                {
                                    vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                                }
                            }
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStrs16u8(&out0, &out1, dstTmp + y * out->addr->stride_y, 8);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * out->addr->stride_y), 8);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            vPrv[conv_height - 1] = vdupq_n_s16(cval.S16);
                            vCur[conv_height - 1] = vPrv[conv_height - 1];
                            vNxt[conv_height - 1] = vPrv[conv_height - 1];
                        }
                        else
                        {
                            if (0 == x)
                            {
                                vPrv[conv_height - 1] = vdupq_n_s16(cval.S16);
                            }
                            else
                            {
                                vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x - 8) * in->addr->stride_x));
                            }
                            vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + x * in->addr->stride_x));
                            if ((x + 8) >= w8)
                            {
                                memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 8) * in->addr->stride_x,
                                       (vxTileWidth(out, 0) - (x + 8)) * sizeof(vx_int16));
                                for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - (x + 8); tmpIdx < 8; tmpIdx++)
                                {
                                    szTmpData[tmpIdx] = cval.S16;
                                }
                                vNxt[conv_height - 1] = vld1q_s16(szTmpData);
                            }
                            else
                            {
                                vNxt[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + (x + 8) * in->addr->stride_x));
                            }
                        }
                    }
                }

                if (w8 < vxTileWidth(out, 0))
                {
                    dstTmp = (vx_uint8 *)dst_base + w8 * out->addr->stride_x;

                    for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                    {
                        if (y < 0)
                        {
                            vPrv[idx] = vdupq_n_s16(cval.S16);
                            vCur[idx] = vPrv[idx];
                            vNxt[idx] = vPrv[idx];
                        }
                        else
                        {
                            vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * in->addr->stride_y + (w8 - 8) * in->addr->stride_x));
                            memcpy(szTmpData, (vx_uint8 *)src_base + y * in->addr->stride_y + w8 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w8) * sizeof(vx_int16));
                            for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - w8; tmpIdx < 8; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = cval.S16;
                            }
                            vCur[idx] = vld1q_s16(szTmpData);
                            vNxt[idx] = vdupq_n_s16(cval.S16);
                        }
                    }

                    for (y = 0; y < high_y; ++y)
                    {
                        out0 = vdupq_n_s32(0);
                        out1 = out0;
                        for (vx_uint8 convY = 0; convY < conv_height; convY++)
                        {
                            if (3 == conv_width)
                            {
                                convRow3x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (5 == conv_width)
                            {
                                convRow5x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (7 == conv_width)
                            {
                                convRow7x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                            else if (9 == conv_width)
                            {
                                convRow9x1s16(&vPrv[convY], &vCur[convY], &vNxt[convY],
                                              conv_mat + (conv_height - (convY + 1)) * conv_width,
                                              &out0, &out1);
                            }
                        }

                        s32ShiftR(&out0, shift);
                        s32ShiftR(&out1, shift);

                        if (dst_format == VX_DF_IMAGE_U8)
                        {
                            convStrs16u8(&out0, &out1, dstTmp + y * out->addr->stride_y, vxTileWidth(out, 0) - w8);
                        }
                        else if (dst_format == VX_DF_IMAGE_S16)
                        {
                            convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * out->addr->stride_y), vxTileWidth(out, 0) - w8);
                        }

                        //swap data and acquire next data
                        for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                        {
                            vPrv[convY] = vPrv[convY + 1];
                            vCur[convY] = vCur[convY + 1];
                            vNxt[convY] = vNxt[convY + 1];
                        }

                        nextY = y + conv_radius_y + 1;
                        if (nextY >= high_y)
                        {
                            vPrv[conv_height - 1] = vdupq_n_s16(cval.S16);
                            vCur[conv_height - 1] = vPrv[conv_height - 1];
                            vNxt[conv_height - 1] = vPrv[conv_height - 1];
                        }
                        else
                        {
                            vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * in->addr->stride_y + (w8 - 8) * in->addr->stride_x));
                            memcpy(szTmpData, (vx_uint8 *)src_base + nextY * in->addr->stride_y + w8 * in->addr->stride_x,
                                   (vxTileWidth(out, 0) - w8) * sizeof(vx_int16));
                            for (vx_uint8 tmpIdx = vxTileWidth(out, 0) - w8; tmpIdx < 8; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = cval.S16;
                            }
                            vCur[conv_height - 1] = vld1q_s16(szTmpData);
                            vNxt[conv_height - 1] = vdupq_n_s16(cval.S16);
                        }
                    }
                }
            }
        }
    }
}


static void vxReadRectangle_flexible(const void *base, const vx_imagepatch_addressing_t *addr, 
                            vx_df_image type, vx_uint32 center_x, vx_uint32 center_y, 
                            vx_uint32 radius_x, vx_uint32 radius_y, void *destination, vx_border_t borders)
{
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 dest_index = 0;
    // kx, ky - kernel x and y
    if (borders.mode == VX_BORDER_REPLICATE || borders.mode == VX_BORDER_UNDEFINED )
    {
        for (ky = -(int32_t)radius_y; ky <= (int32_t)radius_y; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            y = y < 0 ? 0 : y >= height ? height - 1 : y;

            for (kx = -(int32_t)radius_x; kx <= (int32_t)radius_x; ++kx, ++dest_index)
            {
                vx_int32 x = (int32_t)(center_x + kx);
                x = x < 0 ? 0 : x >= width ? width - 1 : x;

                switch (type)
                {
                case VX_DF_IMAGE_U8:
                    ((vx_uint8*)destination)[dest_index] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
                    break;
                case VX_DF_IMAGE_S16:
                case VX_DF_IMAGE_U16:
                    ((vx_uint16*)destination)[dest_index] = *(vx_uint16*)(ptr + y*stride_y + x*stride_x);
                    break;
                }
            }
        }
    }
    else if (borders.mode == VX_BORDER_CONSTANT)
    {
        vx_pixel_value_t cval = borders.constant_value;
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
                }
            }
        }
    }
}

#define CONVOLVE(low_y, high_y, low_x, high_x)                                                                                          \
    for (y = low_y; y < high_y; ++y)                                                                                                    \
    {                                                                                                                                   \
        for (x = low_x; x < high_x; ++x)                                                                                                \
        {                                                                                                                               \
            sum = 0;                                                                                                                    \
            if (src_format == VX_DF_IMAGE_U8)                                                                                           \
            {                                                                                                                           \
                vx_uint8 slice[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };                                                  \
                                                                                                                                        \
                vxReadRectangle_flexible(src_base, in->addr, src_format, x, y, conv_radius_x, conv_radius_y, slice, borders);           \
                                                                                                                                        \
                for (i = 0; i < (vx_int32)(conv_width * conv_height); ++i)                                                              \
                    sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];                                                       \
            }                                                                                                                           \
            else if (src_format == VX_DF_IMAGE_S16)                                                                                     \
            {                                                                                                                           \
                vx_int16 slice[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };                                                  \
                                                                                                                                        \
                vxReadRectangle_flexible(src_base, in->addr, src_format, x, y, conv_radius_x, conv_radius_y, slice, borders);           \
                                                                                                                                        \
                for (i = 0; i < (vx_int32)(conv_width * conv_height); ++i)                                                              \
                    sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];                                                       \
            }                                                                                                                           \
                                                                                                                                        \
            value = sum / (vx_int32)scale;                                                                                              \
                                                                                                                                        \
            if (dst_format == VX_DF_IMAGE_U8)                                                                                           \
            {                                                                                                                           \
                vx_uint8 *dstp = (vx_uint8 *)dst_base + y * out->addr->stride_y + x * out->addr->stride_x;                              \
                if (value < 0) *dstp = 0;                                                                                               \
                else if (value > UINT8_MAX) *dstp = UINT8_MAX;                                                                          \
                else *dstp = value;                                                                                                     \
            }                                                                                                                           \
            else if (dst_format == VX_DF_IMAGE_S16)                                                                                     \
            {                                                                                                                           \
                vx_int16 *dstp = (vx_int16 *)dst_base + y * out->addr->stride_y / 2 + x * out->addr->stride_x / 2;                      \
                if (value < INT16_MIN) *dstp = INT16_MIN;                                                                               \
                else if (value > INT16_MAX) *dstp = INT16_MAX;                                                                          \
                else *dstp = value;                                                                                                     \
            }                                                                                                                           \
        }                                                                                                                               \
    }


void Convolve_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0, i;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_convolution_t *conv = (vx_tile_convolution_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    vx_size conv_width = (*conv).conv_width;
    vx_size conv_height = (*conv).conv_height;

    vx_int32 conv_radius_x, conv_radius_y;

    conv_radius_x = (vx_int32)conv_width / 2;
    conv_radius_y = (vx_int32)conv_height / 2;

    vx_uint32 src_format = in->image.format;
    vx_uint32 dst_format = out->image.format;

    vx_int32 sum = 0, value = 0;

    vx_uint32 scale = (*conv).scale;

    vx_int16 conv_mat[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };

    memcpy(conv_mat, ((*conv).conv_mat), conv_width * conv_height * sizeof(vx_int16));

    vx_border_t borders = in->border;

    if (borders.mode == VX_BORDER_UNDEFINED)
    {
        if (low_y == 0 && low_x == 0)
        {
            CONVOLVE(low_y + conv_radius_y, high_y - conv_radius_y, low_x + conv_radius_x, high_x - conv_radius_x)
        }
        else
        {
            CONVOLVE(conv_radius_y, low_y, low_x, high_x - conv_radius_x)

            src_base = in->base[0];
            dst_base = out->base[0];
            CONVOLVE(low_y, high_y, conv_radius_x, high_x - conv_radius_x)
        }
    }
    else
    {
        if (low_y == 0 && low_x == 0)
        {
            CONVOLVE(low_y, high_y, low_x, high_x)
        }
        else
        {
            CONVOLVE(0, low_y, low_x, high_x)

            src_base = in->base[0];
            dst_base = out->base[0];
            CONVOLVE(low_y, high_y, 0, high_x)
        }
    }
}
