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

#include <venum.h>
#include <VX/vx.h>
#include <arm_neon.h>
#include <string.h>

/**=============================================================================
@function
u32Tou8

@description
algorithm implementation of unint32 to unint8
=============================================================================**/
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


// nodeless version of the Convolve kernel
vx_status vxConvolve(vx_image src, vx_convolution conv, vx_image dst, vx_border_t *bordermode)
{
    vx_int32 y, x, i;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_size conv_width, conv_height;
    vx_int32 conv_radius_x, conv_radius_y;
    vx_int16 conv_mat[VENUM_MAX_CONVOLUTION_DIM * VENUM_MAX_CONVOLUTION_DIM] = {0};
    vx_int32 sum = 0, value = 0;
    vx_uint32 scale = 1;
    vx_df_image src_format = 0;
    vx_df_image dst_format = 0;
    vx_status status  = VX_SUCCESS;
    vx_int32 shift = 0;

    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_COLUMNS, &conv_width, sizeof(conv_width));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_ROWS, &conv_height, sizeof(conv_height));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_SCALE, &scale, sizeof(scale));
    conv_radius_x = (vx_int32)conv_width / 2;
    conv_radius_y = (vx_int32)conv_height / 2;
    status |= vxCopyConvolutionCoefficients(conv, conv_mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxGetValidRegionImage(src, &rect);
    vx_map_id map_id_src = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &map_id_dst, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    shift = (vx_int32)u32Tou8(scale);

    if (bordermode->mode == VX_BORDER_UNDEFINED)
    {
        if (src_format == VX_DF_IMAGE_U8)
        {
            uint8x16_t vPrv[VENUM_MAX_CONVOLUTION_DIM];
            uint8x16_t vCur[VENUM_MAX_CONVOLUTION_DIM];
            uint8x16_t vNxt[VENUM_MAX_CONVOLUTION_DIM];
            int32x4_t out0 = vdupq_n_s32(0);
            int32x4_t out1 = out0;
            int32x4_t out2 = out0;
            int32x4_t out3 = out0;
            vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
            vx_uint32 dstY = conv_radius_y;
            vx_uint8 *dstTmp;
            for (x = 0; x < w16; x+=16)
            {
                dstTmp = (vx_uint8 *)dst_base + x * dst_addr.stride_x;
                dstY = conv_radius_y;
                if (0 == x)
                {
                    for (y = 0; y < conv_height; y++)
                    {
                        vPrv[y] = vdupq_n_u8(0);
                        vCur[y] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x);
                        vNxt[y] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x);
                    }
                }
                else
                {
                    for (y = 0; y < conv_height; y++)
                    {
                        vPrv[y] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x - 16) * src_addr.stride_x);
                        vCur[y] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x);
                        vNxt[y] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x);
                    }
                }

                for (y = conv_height; y < src_addr.dim_y; (++y, dstY++))
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
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * dst_addr.stride_y, 16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y), 16);
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
                        vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x - 16) * src_addr.stride_x);
                    }
                    vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x);
                    vNxt[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x);
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
                    convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * dst_addr.stride_y, 16);
                }
                else if (dst_format == VX_DF_IMAGE_S16)
                {
                    convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y), 16);
                }
            }

            //process left data
            if (w16 < src_addr.dim_x)
            {
                dstTmp = (vx_uint8 *)dst_base + w16 * dst_addr.stride_x;
                dstY = conv_radius_y;

                for (y = 0; y < conv_height; y++)
                {
                    vPrv[y] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (w16 - 16) * src_addr.stride_x);
                    vCur[y] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + w16 * src_addr.stride_x);
                    vNxt[y] = vdupq_n_u8(0);
                }

                for (y = conv_height; y < src_addr.dim_y; (++y, dstY++))
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
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * dst_addr.stride_y,
                                    src_addr.dim_x - w16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3,
                                     (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y),
                                     src_addr.dim_x - w16);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (w16 - 16) * src_addr.stride_x);
                    if ((y + 1) != src_addr.dim_y)
                    {
                        vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + w16 * src_addr.stride_x);
                    }
                    else
                    {
                        vx_uint8 szTmp[16];
                        memcpy(szTmp, (vx_uint8 *)src_base + y * src_addr.stride_y + w16 * src_addr.stride_x,
                               (src_addr.dim_x - w16) * src_addr.stride_x);
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
                    convStru8u8(&out0, &out1, &out2, &out3, dstTmp + dstY * dst_addr.stride_y,
                                src_addr.dim_x - w16);
                }
                else if (dst_format == VX_DF_IMAGE_S16)
                {
                    convStru8s16(&out0, &out1, &out2, &out3,
                                 (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y),
                                 src_addr.dim_x - w16);
                }
            }
        }
        else if (src_format == VX_DF_IMAGE_S16)
        {
            int16x8_t vPrv[VENUM_MAX_CONVOLUTION_DIM];
            int16x8_t vCur[VENUM_MAX_CONVOLUTION_DIM];
            int16x8_t vNxt[VENUM_MAX_CONVOLUTION_DIM];
            int32x4_t out0 = vdupq_n_s32(0);
            int32x4_t out1 = out0;
            vx_uint32 w8 = (src_addr.dim_x >> 3) << 3;
            vx_uint32 dstY = conv_radius_y;
            vx_uint8 *dstTmp;
            for (x = 0; x < w8; x+=8)
            {
                dstTmp = (vx_uint8 *)dst_base + x * dst_addr.stride_x;
                dstY = conv_radius_y;
                if (0 == x)
                {
                    for (y = 0; y < conv_height; y++)
                    {
                        vPrv[y] = vdupq_n_s16(0);
                        vCur[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x));
                        vNxt[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 8) * src_addr.stride_x));
                    }
                }
                else
                {
                    for (y = 0; y < conv_height; y++)
                    {
                        vPrv[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x - 8) * src_addr.stride_x));
                        vCur[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x));
                        vNxt[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 8) * src_addr.stride_x));
                    }
                }

                for (y = conv_height; y < src_addr.dim_y; (++y, dstY++))
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
                        convStrs16u8(&out0, &out1, dstTmp + dstY * dst_addr.stride_y, 8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y), 8);
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
                        vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x - 8) * src_addr.stride_x));
                    }
                    vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x));
                    vNxt[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 8) * src_addr.stride_x));
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
                    convStrs16u8(&out0, &out1, dstTmp + dstY * dst_addr.stride_y, 8);
                }
                else if (dst_format == VX_DF_IMAGE_S16)
                {
                    convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y), 8);
                }
            }

            if (w8 < src_addr.dim_x)
            {
                dstTmp = (vx_uint8 *)dst_base + w8 * dst_addr.stride_x;
                dstY = conv_radius_y;

                for (y = 0; y < conv_height; y++)
                {
                    vPrv[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (w8 - 8) * src_addr.stride_x));
                    vCur[y] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + w8 * src_addr.stride_x));
                    vNxt[y] = vdupq_n_s16(0);
                }

                for (y = conv_height; y < src_addr.dim_y; (++y, dstY++))
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
                        convStrs16u8(&out0, &out1, dstTmp + dstY * dst_addr.stride_y, src_addr.dim_x - w8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y), src_addr.dim_x - w8);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (w8 - 8) * src_addr.stride_x));
                    if ((y + 1) != src_addr.dim_y)
                    {
                        vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + w8 * src_addr.stride_x));
                    }
                    else
                    {
                        vx_int16 szTmp[8];
                        memcpy(szTmp, (vx_uint8 *)src_base + y * src_addr.stride_y + w8 * src_addr.stride_x,
                               (src_addr.dim_x - w8) * src_addr.stride_x);
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
                    convStrs16u8(&out0, &out1, dstTmp + dstY * dst_addr.stride_y, src_addr.dim_x - w8);
                }
                else if (dst_format == VX_DF_IMAGE_S16)
                {
                    convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + dstY * dst_addr.stride_y),
                                  src_addr.dim_x - w8);
                }
            }
        }
    }
    else if (bordermode->mode == VX_BORDER_REPLICATE)
    {
        if (src_format == VX_DF_IMAGE_U8)
        {
            uint8x16_t vPrv[VENUM_MAX_CONVOLUTION_DIM];
            uint8x16_t vCur[VENUM_MAX_CONVOLUTION_DIM];
            uint8x16_t vNxt[VENUM_MAX_CONVOLUTION_DIM];
            int32x4_t out0 = vdupq_n_s32(0);
            int32x4_t out1 = out0;
            int32x4_t out2 = out0;
            int32x4_t out3 = out0;
            vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
            vx_uint32 nextY;
            vx_int32 tmpY;
            vx_uint32 idx;
            vx_uint8 *dstTmp;
            vx_uint8 szTmpData[16];
            for (x = 0; x < w16; x+=16)
            {
                dstTmp = (vx_uint8 *)dst_base + x * dst_addr.stride_x;
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
                                memcpy(szTmpData, (vx_uint8 *)src_base + (x + 16) * src_addr.stride_x,
                                       (src_addr.dim_x - (x + 16)) * sizeof(vx_uint8));
                                memset(&szTmpData[src_addr.dim_x - (x + 16)],
                                       *((vx_uint8 *)src_base + src_addr.dim_x - 1),
                                       16 - (src_addr.dim_x - (x + 16)));
                                vNxt[idx] = vld1q_u8(szTmpData);
                            }
                            else
                            {
                                vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + (x + 16) * src_addr.stride_x);
                            }
                        }
                        else
                        {
                            vPrv[idx] = vdupq_n_u8(*((vx_uint8 *)src_base + y * src_addr.stride_y));
                            vCur[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y);
                            if ((x + 16) >= w16)
                            {
                                memcpy(szTmpData, (vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x,
                                       (src_addr.dim_x - (x + 16)) * sizeof(vx_uint8));
                                memset(&szTmpData[src_addr.dim_x - (x + 16)],
                                       *((vx_uint8 *)src_base + y * src_addr.stride_y + src_addr.dim_x - 1),
                                       16 - (src_addr.dim_x - (x + 16)));
                                vNxt[idx] = vld1q_u8(szTmpData);
                            }
                            else
                            {
                                vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x);
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
                        vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x - 16) * src_addr.stride_x);
                        vCur[idx] = vld1q_u8((vx_uint8 *)src_base + tmpY * src_addr.stride_y + x * src_addr.stride_x);
                        if ((x + 16) >= w16)
                        {
                            memcpy(szTmpData, (vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x + 16) * src_addr.stride_x,
                                   src_addr.dim_x - (x + 16));
                            for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 16); tmpIdx < 16; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = *((vx_uint8 *)src_base + tmpY * src_addr.stride_y + src_addr.dim_x - 1);
                            }
                            vNxt[idx] = vld1q_u8(szTmpData);
                        }
                        else
                        {
                            vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x + 16) * src_addr.stride_x);
                        }
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * dst_addr.stride_y, 16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + y * dst_addr.stride_y), 16);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
                    {
                        nextY = src_addr.dim_y - 1;
                    }

                    if (0 == x)
                    {
                        vPrv[conv_height - 1] = vdupq_n_u8(*((vx_uint8 *)src_base + nextY * src_addr.stride_y));
                    }
                    else
                    {
                        vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x - 16) * src_addr.stride_x);
                    }
                    vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + x * src_addr.stride_x);
                    if ((x + 16) >= w16)
                    {
                        memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 16) * src_addr.stride_x,
                               src_addr.dim_x - (x + 16));
                        for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 16); tmpIdx < 16; tmpIdx++)
                        {
                            szTmpData[tmpIdx] = *((vx_uint8 *)src_base + nextY * src_addr.stride_y + src_addr.dim_x - 1);
                        }
                        vNxt[conv_height - 1] = vld1q_u8(szTmpData);
                    }
                    else
                    {
                        vNxt[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 16) * src_addr.stride_x);
                    }
                }
            }

            //process left data
            if (w16 < src_addr.dim_x)
            {
                dstTmp = (vx_uint8 *)dst_base + w16 * dst_addr.stride_x;

                for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                {
                    if (y < 0)
                    {
                        vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + (w16 - 16) * src_addr.stride_x);
                        memcpy(szTmpData, (vx_uint8 *)src_base + w16 * src_addr.stride_x,
                               (src_addr.dim_x - w16) * sizeof(vx_uint8));
                        memset(&szTmpData[src_addr.dim_x - w16], *((vx_uint8 *)src_base + src_addr.dim_x - 1),
                               16 - (src_addr.dim_x - w16));
                        vCur[idx] = vld1q_u8(szTmpData);
                        vNxt[idx] = vdupq_n_u8(szTmpData[15]);
                    }
                    else
                    {
                        vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (w16 - 16) * src_addr.stride_x);
                        memcpy(szTmpData, (vx_uint8 *)src_base + y * src_addr.stride_y + w16 * src_addr.stride_x,
                               (src_addr.dim_x - w16) * sizeof(vx_uint8));
                        memset(&szTmpData[src_addr.dim_x - w16], *((vx_uint8 *)src_base + y * src_addr.stride_y + src_addr.dim_x - 1),
                               16 - (src_addr.dim_x - w16));
                        vCur[idx] = vld1q_u8(szTmpData);
                        vNxt[idx] = vdupq_n_u8(szTmpData[15]);
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * dst_addr.stride_y,
                                    src_addr.dim_x - w16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3,
                                     (vx_int16 *)(dstTmp + y * dst_addr.stride_y),
                                     src_addr.dim_x - w16);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
                    {
                        nextY = src_addr.dim_y - 1;
                    }

                    vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + (w16 - 16) * src_addr.stride_x);
                    memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + w16 * src_addr.stride_x,
                           (src_addr.dim_x - w16) * sizeof(vx_uint8));
                    memset(&szTmpData[src_addr.dim_x - w16], *((vx_uint8 *)src_base + nextY * src_addr.stride_y + src_addr.dim_x - 1),
                           16 - (src_addr.dim_x - w16));
                    vCur[conv_height - 1] = vld1q_u8(szTmpData);
                    vNxt[conv_height - 1] = vdupq_n_u8(szTmpData[15]);
                }
            }
        }
        else if (src_format == VX_DF_IMAGE_S16)
        {
            int16x8_t vPrv[VENUM_MAX_CONVOLUTION_DIM];
            int16x8_t vCur[VENUM_MAX_CONVOLUTION_DIM];
            int16x8_t vNxt[VENUM_MAX_CONVOLUTION_DIM];
            int32x4_t out0 = vdupq_n_s32(0);
            int32x4_t out1 = out0;
            vx_uint32 w8 = (src_addr.dim_x >> 3) << 3;
            vx_uint32 idx;
            vx_uint32 nextY;
            vx_int32 tmpY;
            vx_uint8 *dstTmp;
            vx_int16 szTmpData[8];
            for (x = 0; x < w8; x+=8)
            {
                dstTmp = (vx_uint8 *)dst_base + x * dst_addr.stride_x;
                if (0 == x)
                {
                    for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                    {
                        if (y < 0)
                        {
                            vPrv[idx] = vdupq_n_s16(*(vx_int16 *)src_base);
                            vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + x * src_addr.stride_x));
                            vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + (x + 8) * src_addr.stride_x));
                        }
                        else
                        {
                            vPrv[idx] = vdupq_n_s16(*(vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y));
                            vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x));
                            vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 8) * src_addr.stride_x));
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
                        vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x - 8) * src_addr.stride_x));
                        vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + tmpY * src_addr.stride_y + x * src_addr.stride_x));
                        if ((x + 8) >= w8)
                        {
                            memcpy(szTmpData, (vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x + 8) * src_addr.stride_x,
                                   (src_addr.dim_x - (x + 8)) * sizeof(vx_int16));
                            for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 8); tmpIdx < 8; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + tmpY * src_addr.stride_y + src_addr.dim_x - 2);
                            }
                            vNxt[idx] = vld1q_s16(szTmpData);
                        }
                        else
                        {
                            vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x + 8) * src_addr.stride_x));
                        }
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStrs16u8(&out0, &out1, dstTmp + y * dst_addr.stride_y, 8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * dst_addr.stride_y), 8);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
                    {
                        nextY = src_addr.dim_y - 1;
                    }

                    if (0 == x)
                    {
                        vPrv[conv_height - 1] = vdupq_n_s16(*(vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y));
                    }
                    else
                    {
                        vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x - 8) * src_addr.stride_x));
                    }
                    vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + x * src_addr.stride_x));
                    if ((x + 8) >= w8)
                    {
                        memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 8) * src_addr.stride_x,
                               (src_addr.dim_x - (x + 8)) * sizeof(vx_int16));
                        for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 8); tmpIdx < 8; tmpIdx++)
                        {
                            szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + src_addr.dim_x - 2);
                        }
                        vNxt[conv_height - 1] = vld1q_s16(szTmpData);
                    }
                    else
                    {
                        vNxt[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 8) * src_addr.stride_x));
                    }
                }
            }

            if (w8 < src_addr.dim_x)
            {
                dstTmp = (vx_uint8 *)dst_base + w8 * dst_addr.stride_x;

                for (y = -conv_radius_y, idx = 0; y <= conv_radius_y; (y++, idx++))
                {
                    if (y < 0)
                    {
                        vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + (w8 - 8) * src_addr.stride_x));
                        memcpy(szTmpData, (vx_uint8 *)src_base + w8 * src_addr.stride_x,
                               (src_addr.dim_x - w8) * sizeof(vx_int16));
                        for (vx_uint8 tmpIdx = src_addr.dim_x - w8; tmpIdx < 8; tmpIdx++)
                        {
                            szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + src_addr.dim_x - 2);
                        }
                        vCur[idx] = vld1q_s16(szTmpData);
                        vNxt[idx] = vdupq_n_s16(szTmpData[7]);
                    }
                    else
                    {
                        vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (w8 - 8) * src_addr.stride_x));
                        memcpy(szTmpData, (vx_uint8 *)src_base + y * src_addr.stride_y + w8 * src_addr.stride_x,
                               (src_addr.dim_x - w8) * sizeof(vx_int16));
                        for (vx_uint8 tmpIdx = src_addr.dim_x - w8; tmpIdx < 8; tmpIdx++)
                        {
                            szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + src_addr.dim_x - 2);
                        }
                        vCur[idx] = vld1q_s16(szTmpData);
                        vNxt[idx] = vdupq_n_s16(szTmpData[7]);
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStrs16u8(&out0, &out1, dstTmp + y * dst_addr.stride_y, src_addr.dim_x - w8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * dst_addr.stride_y), src_addr.dim_x - w8);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
                    {
                        nextY = src_addr.dim_y - 1;
                    }

                    vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + (w8 - 8) * src_addr.stride_x));
                    memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + w8 * src_addr.stride_x,
                           (src_addr.dim_x - w8) * sizeof(vx_int16));
                    for (vx_uint8 tmpIdx = src_addr.dim_x - w8; tmpIdx < 8; tmpIdx++)
                    {
                        szTmpData[tmpIdx] = *(vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + src_addr.dim_x - 2);
                    }
                    vCur[conv_height - 1] = vld1q_s16(szTmpData);
                    vNxt[conv_height - 1] = vdupq_n_s16(szTmpData[7]);
                }
            }
        }
    }
    else if (bordermode->mode == VX_BORDER_CONSTANT)
    {
        if (src_format == VX_DF_IMAGE_U8)
        {
            uint8x16_t vPrv[VENUM_MAX_CONVOLUTION_DIM];
            uint8x16_t vCur[VENUM_MAX_CONVOLUTION_DIM];
            uint8x16_t vNxt[VENUM_MAX_CONVOLUTION_DIM];
            int32x4_t out0 = vdupq_n_s32(0);
            int32x4_t out1 = out0;
            int32x4_t out2 = out0;
            int32x4_t out3 = out0;
            vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
            vx_pixel_value_t cval = bordermode->constant_value;
            vx_uint32 nextY;
            vx_uint32 idx;
            vx_uint8 *dstTmp;
            vx_uint8 szTmpData[16];
            for (x = 0; x < w16; x+=16)
            {
                dstTmp = (vx_uint8 *)dst_base + x * dst_addr.stride_x;
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
                            vCur[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y);
                            vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x);
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
                            vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x - 16) * src_addr.stride_x);
                            vCur[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x);
                            if ((x + 16) >= w16)
                            {
                                memcpy(szTmpData, (vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x,
                                       src_addr.dim_x - (x + 16));
                                for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 16); tmpIdx < 16; tmpIdx++)
                                {
                                    szTmpData[tmpIdx] = cval.U8;
                                }
                                vNxt[idx] = vld1q_u8(szTmpData);
                            }
                            else
                            {
                                vNxt[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 16) * src_addr.stride_x);
                            }
                        }
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * dst_addr.stride_y, 16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3, (vx_int16 *)(dstTmp + y * dst_addr.stride_y), 16);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
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
                            vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x - 16) * src_addr.stride_x);
                        }
                        vCur[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + x * src_addr.stride_x);
                        if ((x + 16) >= w16)
                        {
                            memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 16) * src_addr.stride_x,
                                   src_addr.dim_x - (x + 16));
                            for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 16); tmpIdx < 16; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = cval.U8;
                            }
                            vNxt[conv_height - 1] = vld1q_u8(szTmpData);
                        }
                        else
                        {
                            vNxt[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 16) * src_addr.stride_x);
                        }
                    }
                }
            }

            //process left data
            if (w16 < src_addr.dim_x)
            {
                dstTmp = (vx_uint8 *)dst_base + w16 * dst_addr.stride_x;

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
                        vPrv[idx] = vld1q_u8((vx_uint8 *)src_base + y * src_addr.stride_y + (w16 - 16) * src_addr.stride_x);
                        memcpy(szTmpData, (vx_uint8 *)src_base + y * src_addr.stride_y + w16 * src_addr.stride_x,
                               (src_addr.dim_x - w16) * sizeof(vx_uint8));
                        memset(&szTmpData[src_addr.dim_x - w16], cval.U8,
                               16 - (src_addr.dim_x - w16));
                        vCur[idx] = vld1q_u8(szTmpData);
                        vNxt[idx] = vdupq_n_u8(cval.U8);
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStru8u8(&out0, &out1, &out2, &out3, dstTmp + y * dst_addr.stride_y,
                                    src_addr.dim_x - w16);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStru8s16(&out0, &out1, &out2, &out3,
                                     (vx_int16 *)(dstTmp + y * dst_addr.stride_y),
                                     src_addr.dim_x - w16);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
                    {
                        vPrv[conv_height - 1] = vdupq_n_u8(cval.U8);
                        vCur[conv_height - 1] = vPrv[conv_height - 1];
                        vNxt[conv_height - 1] = vPrv[conv_height - 1];
                    }
                    else
                    {
                        vPrv[conv_height - 1] = vld1q_u8((vx_uint8 *)src_base + nextY * src_addr.stride_y + (w16 - 16) * src_addr.stride_x);
                        memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + w16 * src_addr.stride_x,
                               (src_addr.dim_x - w16) * sizeof(vx_uint8));
                        memset(&szTmpData[src_addr.dim_x - w16], cval.U8,
                               16 - (src_addr.dim_x - w16));
                        vCur[conv_height - 1] = vld1q_u8(szTmpData);
                        vNxt[conv_height - 1] = vdupq_n_u8(cval.U8);
                    }
                }
            }
        }
        else if (src_format == VX_DF_IMAGE_S16)
        {
            int16x8_t vPrv[VENUM_MAX_CONVOLUTION_DIM];
            int16x8_t vCur[VENUM_MAX_CONVOLUTION_DIM];
            int16x8_t vNxt[VENUM_MAX_CONVOLUTION_DIM];
            int32x4_t out0 = vdupq_n_s32(0);
            int32x4_t out1 = out0;
            vx_uint32 w8 = (src_addr.dim_x >> 3) << 3;
            vx_pixel_value_t cval = bordermode->constant_value;
            vx_uint32 idx;
            vx_uint32 nextY;
            vx_uint8 *dstTmp;
            vx_int16 szTmpData[8];
            for (x = 0; x < w8; x+=8)
            {
                dstTmp = (vx_uint8 *)dst_base + x * dst_addr.stride_x;
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
                            vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x));
                            vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 8) * src_addr.stride_x));
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
                            vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x - 8) * src_addr.stride_x));
                            vCur[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + x * src_addr.stride_x));
                            if ((x + 8) >= w8)
                            {
                                memcpy(szTmpData, (vx_uint8 *)src_base + y * src_addr.stride_y + (x + 8) * src_addr.stride_x,
                                       (src_addr.dim_x - (x + 8)) * sizeof(vx_int16));
                                for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 8); tmpIdx < 8; tmpIdx++)
                                {
                                    szTmpData[tmpIdx] = cval.S16;
                                }
                                vNxt[idx] = vld1q_s16(szTmpData);
                            }
                            else
                            {
                                vNxt[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (x + 8) * src_addr.stride_x));
                            }
                        }
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStrs16u8(&out0, &out1, dstTmp + y * dst_addr.stride_y, 8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * dst_addr.stride_y), 8);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
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
                            vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x - 8) * src_addr.stride_x));
                        }
                        vCur[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + x * src_addr.stride_x));
                        if ((x + 8) >= w8)
                        {
                            memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 8) * src_addr.stride_x,
                                   (src_addr.dim_x - (x + 8)) * sizeof(vx_int16));
                            for (vx_uint8 tmpIdx = src_addr.dim_x - (x + 8); tmpIdx < 8; tmpIdx++)
                            {
                                szTmpData[tmpIdx] = cval.S16;
                            }
                            vNxt[conv_height - 1] = vld1q_s16(szTmpData);
                        }
                        else
                        {
                            vNxt[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + (x + 8) * src_addr.stride_x));
                        }
                    }
                }
            }

            if (w8 < src_addr.dim_x)
            {
                dstTmp = (vx_uint8 *)dst_base + w8 * dst_addr.stride_x;

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
                        vPrv[idx] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + y * src_addr.stride_y + (w8 - 8) * src_addr.stride_x));
                        memcpy(szTmpData, (vx_uint8 *)src_base + y * src_addr.stride_y + w8 * src_addr.stride_x,
                               (src_addr.dim_x - w8) * sizeof(vx_int16));
                        for (vx_uint8 tmpIdx = src_addr.dim_x - w8; tmpIdx < 8; tmpIdx++)
                        {
                            szTmpData[tmpIdx] = cval.S16;
                        }
                        vCur[idx] = vld1q_s16(szTmpData);
                        vNxt[idx] = vdupq_n_s16(cval.S16);
                    }
                }

                for (y = 0; y < src_addr.dim_y; ++y)
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
                        convStrs16u8(&out0, &out1, dstTmp + y * dst_addr.stride_y, src_addr.dim_x - w8);
                    }
                    else if (dst_format == VX_DF_IMAGE_S16)
                    {
                        convStrs16s16(&out0, &out1, (vx_int16 *)(dstTmp + y * dst_addr.stride_y), src_addr.dim_x - w8);
                    }

                    //swap data and acquire next data
                    for (vx_uint8 convY = 0; convY < (conv_height - 1); convY++)
                    {
                        vPrv[convY] = vPrv[convY + 1];
                        vCur[convY] = vCur[convY + 1];
                        vNxt[convY] = vNxt[convY + 1];
                    }

                    nextY = y + conv_radius_y + 1;
                    if (nextY >= src_addr.dim_y)
                    {
                        vPrv[conv_height - 1] = vdupq_n_s16(cval.S16);
                        vCur[conv_height - 1] = vPrv[conv_height - 1];
                        vNxt[conv_height - 1] = vPrv[conv_height - 1];
                    }
                    else
                    {
                        vPrv[conv_height - 1] = vld1q_s16((vx_int16 *)((vx_uint8 *)src_base + nextY * src_addr.stride_y + (w8 - 8) * src_addr.stride_x));
                        memcpy(szTmpData, (vx_uint8 *)src_base + nextY * src_addr.stride_y + w8 * src_addr.stride_x,
                               (src_addr.dim_x - w8) * sizeof(vx_int16));
                        for (vx_uint8 tmpIdx = src_addr.dim_x - w8; tmpIdx < 8; tmpIdx++)
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
    vxUnmapImagePatch(src, map_id_src);
    vxUnmapImagePatch(dst, map_id_dst);
    return status;
}

