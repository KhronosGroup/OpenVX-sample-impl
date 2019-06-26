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
#include <arm_neon.h>
#include <vx_debug.h>

// helpers -------------------------------------------------------------------

static vx_uint8 usat8(vx_int32 a)
{
    if (a > 255)
        a = 255;
    if (a < 0)
        a = 0;
    return (vx_uint8)a;
}

static void yuv2rgb_bt601(vx_uint8 y, vx_uint8 cb, vx_uint8 cr,
                          vx_uint8 *r, vx_uint8 *g, vx_uint8 *b)
{
    /*
    R'= Y' + 0.000*U' + 1.403*V'
    G'= Y' - 0.344*U' - 0.714*V'
    B'= Y' + 1.773*U' + 0.000*V'
    */
    vx_float64 f_y = (vx_float64)y;
    vx_float64 f_u = (vx_float64)cb - 128;
    vx_float64 f_v = (vx_float64)cr - 128;
    vx_float64 f_r = f_y + 0.000f*f_u + 1.403f*f_v;
    vx_float64 f_g = f_y - 0.344f*f_u - 0.714f*f_v;
    vx_float64 f_b = f_y + 1.773f*f_u + 0.000f*f_v;
    vx_int32 i_r = (vx_int32)f_r;
    vx_int32 i_g = (vx_int32)f_g;
    vx_int32 i_b = (vx_int32)f_b;
    *r = usat8(i_r);
    *g = usat8(i_g);
    *b = usat8(i_b);
}

static void yuv2rgb_bt709(vx_uint8 y, vx_uint8 cb, vx_uint8 cr,
                          vx_uint8 *r, vx_uint8 *g, vx_uint8 *b)
{
    /*
    R'= Y' + 0.0000*U + 1.5748*V
    G'= Y' - 0.1873*U - 0.4681*V
    B'= Y' + 1.8556*U + 0.0000*V
    */
    vx_float64 f_y = (vx_float64)y;
    vx_float64 f_u = (vx_float64)cb - 128;
    vx_float64 f_v = (vx_float64)cr - 128;
    vx_float64 f_r = f_y + 0.0000f*f_u + 1.5748f*f_v;
    vx_float64 f_g = f_y - 0.1873f*f_u - 0.4681f*f_v;
    vx_float64 f_b = f_y + 1.8556f*f_u + 0.0000f*f_v;
    vx_int32 i_r = (vx_int32)f_r;
    vx_int32 i_g = (vx_int32)f_g;
    vx_int32 i_b = (vx_int32)f_b;
    *r = usat8(i_r);
    *g = usat8(i_g);
    *b = usat8(i_b);
}


static void rgb2yuv_bt709(vx_uint8 r, vx_uint8 g, vx_uint8 b,
                          vx_uint8 *y, vx_uint8 *cb, vx_uint8 *cr)
{
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    U'=-0.1146*R' - 0.3854*G' + 0.5000*B'
    V'= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */
    vx_float64 f_r = (vx_float64)r;
    vx_float64 f_g = (vx_float64)g;
    vx_float64 f_b = (vx_float64)b;
    vx_float64 f_y = 0 + 0.2126f*f_r + 0.7152f*f_g + 0.0722f*f_b;
    vx_float64 f_u = 0 - 0.1146f*f_r - 0.3854f*f_g + 0.5000f*f_b;
    vx_float64 f_v = 0 + 0.5000f*f_r - 0.4542f*f_g - 0.0458f*f_b;
    vx_int32 i_y = (vx_int32)f_y;
    vx_int32 i_u = (vx_int32)f_u + 128;
    vx_int32 i_v = (vx_int32)f_v + 128;
    *y  = usat8(i_y);
    *cb = usat8(i_u);
    *cr = usat8(i_v);
}

static void yuv2yuv_601to709(vx_uint8 y0, vx_uint8 cb0, vx_uint8 cr0,
                             vx_uint8 *y1, vx_uint8 *cb1, vx_uint8 *cr1)
{
    /*
    Y' = 1.0090*Y - 0.11826430*Cb - 0.2000311*Cr
    Cb'= 0.0000*Y + 1.01911200*Cb + 0.1146035*Cr
    Cr'= 0.0001*Y + 0.07534570*Cb + 1.0290932*Cr
    */
    vx_float64 f_y0 = (vx_float64)y0;
    vx_float64 f_cb0 = (vx_float64)cb0;
    vx_float64 f_cr0 = (vx_float64)cr0;
    vx_float64 f_y1  = 1.0090*f_y0 - 0.11826430*f_cb0 - 0.2000311*f_cr0;
    vx_float64 f_cb1 = 0.0000*f_y0 + 1.01911200*f_cb0 + 0.1146035*f_cr0;
    vx_float64 f_cr1 = 0.0001*f_y0 + 0.07534570*f_cb0 + 1.0290932*f_cr0;
    vx_int32 i_y = (vx_int32)f_y1;
    vx_int32 i_cb = (vx_int32)f_cb1;
    vx_int32 i_cr = (vx_int32)f_cr1;
    *y1 = usat8(i_y);
    *cb1 = usat8(i_cb);
    *cr1 = usat8(i_cr);
}

static void rgb2yuv_bt709_neon(vx_float32 *arrfr, vx_float32 *arrfg, vx_float32 *arrfb,
    vx_uint8 **y, vx_uint8 *cb, vx_uint8 *cr)
{
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    U'=-0.1146*R' - 0.3854*G' + 0.5000*B'
    V'= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */

    float32x4_t fr32x4 = vld1q_f32(arrfr);
    float32x4_t fg32x4 = vld1q_f32(arrfg);
    float32x4_t fb32x4 = vld1q_f32(arrfb);

    float32x4_t fy32x4 = vdupq_n_f32(0.0f);               
    fy32x4 = vmlaq_n_f32(fy32x4, fr32x4, 0.2126f); 
    fy32x4 = vmlaq_n_f32(fy32x4, fg32x4, 0.7152f); 
    fy32x4 = vmlaq_n_f32(fy32x4, fb32x4, 0.0722f); 

    float32x4_t fu32x4 =  vdupq_n_f32(0.0f);
    fu32x4 = vmlaq_n_f32(fu32x4, fr32x4, -0.1146f); 
    fu32x4 = vmlaq_n_f32(fu32x4, fg32x4, -0.3854f); 
    fu32x4 = vmlaq_n_f32(fu32x4, fb32x4, 0.5000f); 

    float32x4_t fv32x4 = vdupq_n_f32(0.0f);
    fv32x4 = vmlaq_n_f32(fv32x4, fr32x4, 0.5000f); 
    fv32x4 = vmlaq_n_f32(fv32x4, fg32x4, -0.4542f); 
    fv32x4 = vmlaq_n_f32(fv32x4, fb32x4, -0.0458f); 

    int32x4_t iy32x4 = vcvtq_s32_f32(fy32x4); 

    int32x4_t icoeff32x4 = vdupq_n_s32(128);                    
    int32x4_t iu32x4 = vcvtq_s32_f32(fu32x4);
    iu32x4 = vaddq_s32(iu32x4, icoeff32x4);

    int32x4_t iv32x4 = vcvtq_s32_f32(fv32x4);
    iv32x4 = vaddq_s32(iv32x4, icoeff32x4);

    int16x4_t vqmovn_s32 (int32x4_t __a);
    uint16x4_t vreinterpret_u16_s16 (int16x4_t __a); 
    uint8x8_t vqmovn_u16 (uint16x8_t __a);

    y[0][0] = usat8(vgetq_lane_s32(iy32x4, 0));
    y[1][0] = usat8(vgetq_lane_s32(iy32x4, 1));
    y[2][0] = usat8(vgetq_lane_s32(iy32x4, 2));
    y[3][0] = usat8(vgetq_lane_s32(iy32x4, 3));


    cb[0] = usat8(vgetq_lane_s32(iu32x4, 0));
    cb[1] = usat8(vgetq_lane_s32(iu32x4, 1));
    cb[2] = usat8(vgetq_lane_s32(iu32x4, 2));
    cb[3] = usat8(vgetq_lane_s32(iu32x4, 3));

    cr[0] = usat8(vgetq_lane_s32(iv32x4, 0));
    cr[1] = usat8(vgetq_lane_s32(iv32x4, 1));
    cr[2] = usat8(vgetq_lane_s32(iv32x4, 2));
    cr[3] = usat8(vgetq_lane_s32(iv32x4, 3));
}

static void yuv2rgb_bt601_neon(vx_uint8 **y, vx_uint8 cb, vx_uint8 cr,
    vx_uint8 **r, vx_uint8 **g, vx_uint8 **b)
{
    /*
    R'= Y' + 0.000*U' + 1.403*V'
    G'= Y' - 0.344*U' - 0.714*V'
    B'= Y' + 1.773*U' + 0.000*V'
    */
    vx_float32 fy[4] = { (vx_float32)y[0][0], (vx_float32)y[1][0], (vx_float32)y[2][0], (vx_float32)y[3][0] };
    vx_float32 fu[4] = { (vx_float32)cb - 128, (vx_float32)cb - 128, (vx_float32)cb - 128, (vx_float32)cb - 128 };
    vx_float32 fv[4] = { (vx_float32)cr - 128, (vx_float32)cr - 128, (vx_float32)cr - 128, (vx_float32)cr - 128 };

    float32x4_t fy32x4 = vld1q_f32(fy);
    float32x4_t fu32x4 = vld1q_f32(fu);
    float32x4_t fv32x4 = vld1q_f32(fv);

    float32x4_t fr32x4 = vdupq_n_f32(0.0f);               
    fr32x4 = vaddq_f32(fr32x4, fy32x4);
    fr32x4 = vmlaq_n_f32(fr32x4, fu32x4, 0.000f); 
    fr32x4 = vmlaq_n_f32(fr32x4, fv32x4, 1.403f); 

    float32x4_t fg32x4 = vdupq_n_f32(0.0f);               
    fg32x4 = vaddq_f32(fg32x4, fy32x4);
    fg32x4 = vmlaq_n_f32(fg32x4, fu32x4, -0.344f); 
    fg32x4 = vmlaq_n_f32(fg32x4, fv32x4, -0.714f); 

    float32x4_t fb32x4 = vdupq_n_f32(0.0f);               
    fb32x4 = vaddq_f32(fb32x4, fy32x4);
    fb32x4 = vmlaq_n_f32(fb32x4, fu32x4, 1.773f); 
    fb32x4 = vmlaq_n_f32(fb32x4, fv32x4, 0.000f); 

    int32x4_t ir32x4 = vcvtq_s32_f32(fr32x4); 
    int32x4_t ig32x4 = vcvtq_s32_f32(fg32x4);
    int32x4_t ib32x4 = vcvtq_s32_f32(fb32x4);

    int32_t arr32[12]; 
    vst1q_s32(arr32,  ir32x4);
    vst1q_s32(arr32+4, ig32x4);
    vst1q_s32(arr32+8, ib32x4);

    for (vx_uint8 i = 0; i < 4; i++)
    {
        r[i][0] = usat8(arr32[i]);
        g[i][1] = usat8(arr32[4 + i]);
        b[i][2] = usat8(arr32[8 + i]);
    }
}

static void yuv2rgb_bt709_neon(vx_uint8 **y, vx_uint8 cb, vx_uint8 cr,
    vx_uint8 **r, vx_uint8 **g, vx_uint8 **b)
{
    /*
    R'= Y' + 0.0000*U + 1.5748*V
    G'= Y' - 0.1873*U - 0.4681*V
    B'= Y' + 1.8556*U + 0.0000*V
    */
    vx_float32 fy[4] = { (vx_float32)y[0][0], (vx_float32)y[1][0], (vx_float32)y[2][0], (vx_float32)y[3][0] };
    vx_float32 fu[4] = { (vx_float32)cb - 128, (vx_float32)cb - 128, (vx_float32)cb - 128, (vx_float32)cb - 128 };
    vx_float32 fv[4] = { (vx_float32)cr - 128, (vx_float32)cr - 128, (vx_float32)cr - 128, (vx_float32)cr - 128 };

    float32x4_t fy32x4 = vld1q_f32(fy);
    float32x4_t fu32x4 = vld1q_f32(fu);
    float32x4_t fv32x4 = vld1q_f32(fv);

    float32x4_t fr32x4 = vdupq_n_f32(0.0f);               
    fr32x4 = vaddq_f32(fr32x4, fy32x4);
    fr32x4 = vmlaq_n_f32(fr32x4, fu32x4, 0.000f); 
    fr32x4 = vmlaq_n_f32(fr32x4, fv32x4, 1.5748f); 

    float32x4_t fg32x4 = vdupq_n_f32(0.0f);               
    fg32x4 = vaddq_f32(fg32x4, fy32x4);
    fg32x4 = vmlaq_n_f32(fg32x4, fu32x4, -0.1873f);
    fg32x4 = vmlaq_n_f32(fg32x4, fv32x4,  -0.4681f);

    float32x4_t fb32x4 = vdupq_n_f32(0.0f);
    fb32x4 = vaddq_f32(fb32x4, fy32x4);
    fb32x4 = vmlaq_n_f32(fb32x4, fu32x4, 1.8556f);
    fb32x4 = vmlaq_n_f32(fb32x4, fv32x4, 0.000f);

    int32x4_t ir32x4 = vcvtq_s32_f32(fr32x4); 
    int32x4_t ig32x4 = vcvtq_s32_f32(fg32x4);
    int32x4_t ib32x4 = vcvtq_s32_f32(fb32x4);

    int32_t arr32[12]; 
    vst1q_s32(arr32, ir32x4);
    vst1q_s32(arr32 + 4, ig32x4);
    vst1q_s32(arr32+8, ib32x4);

    for (vx_uint8 i = 0; i < 4; i++)
    {
        r[i][0] = usat8(arr32[i]);
        g[i][1] = usat8(arr32[4 + i]);
        b[i][2] = usat8(arr32[8 + i]);
    }
}

static void yuv2yuv_601to709_neon(vx_uint8 *y0, vx_uint8 *cb0, vx_uint8 *cr0,
    vx_uint8 *y1, vx_uint8 *cb1, vx_uint8 *cr1)
{
    /*
    Y' = 1.0090*Y - 0.11826430*Cb - 0.2000311*Cr
    Cb'= 0.0000*Y + 1.01911200*Cb + 0.1146035*Cr
    Cr'= 0.0001*Y + 0.07534570*Cb + 1.0290932*Cr
    */
    vx_float32 fy0[4] = { (vx_float32)y0[0], (vx_float32)y0[1], (vx_float32)y0[2], (vx_float32)y0[3] };
    vx_float32 fcb0[4] = { (vx_float32)cb0[0], (vx_float32)cb0[1], (vx_float32)cb0[2], (vx_float32)cb0[3] };
    vx_float32 fcr0[4] = { (vx_float32)cr0[0], (vx_float32)cr0[1], (vx_float32)cr0[2], (vx_float32)cr0[3] };;

    float32x4_t fy032x4  = vld1q_f32(fy0);
    float32x4_t fcb032x4 = vld1q_f32(fcb0);
    float32x4_t fcr032x4 = vld1q_f32(fcr0);

    float32x4_t fy132x4 = vdupq_n_f32(0.0f);               
    fy132x4 = vmlaq_n_f32(fy132x4, fy032x4, 1.0090); 
    fy132x4 = vmlaq_n_f32(fy132x4, fcb032x4, -0.11826430); 
    fy132x4 = vmlaq_n_f32(fy132x4, fcr032x4, -0.2000311); 

    float32x4_t fcb132x4 = vdupq_n_f32(0.0f);               
    fcb132x4 = vmlaq_n_f32(fcb132x4, fy032x4, 0.0000); 
    fcb132x4 = vmlaq_n_f32(fcb132x4, fcb032x4, 1.01911200); 
    fcb132x4 = vmlaq_n_f32(fcb132x4, fcr032x4, 0.1146035); 

    float32x4_t fcr132x4 = vdupq_n_f32(0.0f);               
    fcr132x4 = vmlaq_n_f32(fcr132x4, fy032x4, 0.0001); 
    fcr132x4 = vmlaq_n_f32(fcr132x4, fcb032x4, 0.07534570); 
    fcr132x4 = vmlaq_n_f32(fcr132x4, fcr032x4,  1.0290932); 

    int32x4_t iy32x4 = vcvtq_s32_f32(fy132x4); 
    int32x4_t icb32x4 = vcvtq_s32_f32(fcb132x4); 
    int32x4_t icr32x4 = vcvtq_s32_f32(fcr132x4); 

    int32_t arr32[12]; 
    vst1q_s32(arr32,  iy32x4);
    vst1q_s32(arr32+4, icb32x4);
    vst1q_s32(arr32+8, icr32x4);

    for(vx_uint8 i = 0; i < 4; i++) 
    {
        y1[i]  = usat8(arr32[i]); 
        cb1[2*i] = usat8(arr32[4 + i]);
        cr1[2*i+1] = usat8(arr32[8 + i]); 
    }
}


static void yuv2rgb_bt601V(vx_float32* y, vx_float32* cb, vx_float32* cr,
    vx_uint8 *rUint8, vx_uint8 *gUint8, vx_uint8 *bUint8)
{
    float32x4_t y32X4Value = vld1q_f32(y);
    float32x4_t cb32X4Value = vld1q_f32(cb);
    float32x4_t cr32X4Value = vld1q_f32(cr);
    float32x4_t All128 = vdupq_n_f32(128.0f);
    float32x4_t AllZero = vdupq_n_f32(0.0f);
    float32x4_t rFloatValue, gFloatValue, bFloatValue;
    int32x4_t  rIntValue, gIntValue, bIntValue;
    cb32X4Value = vsubq_f32(cb32X4Value,All128);
    cr32X4Value = vsubq_f32(cr32X4Value,All128);

    // R'= Y' + 0.000*U' + 1.403*V'
    // G'= Y' - 0.344*U' - 0.714*V'
    // B'= Y' + 1.773*U' + 0.000*V'
    rFloatValue = vmlaq_n_f32(y32X4Value, cr32X4Value, 1.403f);

    gFloatValue = vmlaq_n_f32(y32X4Value, cb32X4Value, -0.344f);
    gFloatValue = vmlaq_n_f32(gFloatValue, cr32X4Value, -0.714f);

    bFloatValue = vmlaq_n_f32(y32X4Value, cb32X4Value, 1.773f);

    rIntValue = vcvtq_s32_f32(rFloatValue);
    gIntValue = vcvtq_s32_f32(gFloatValue);
    bIntValue = vcvtq_s32_f32(bFloatValue);

    rUint8[0] = usat8(vgetq_lane_s32(rIntValue, 0));
    gUint8[0] = usat8(vgetq_lane_s32(gIntValue, 0));
    bUint8[0] = usat8(vgetq_lane_s32(bIntValue, 0));

    rUint8[1] = usat8(vgetq_lane_s32(rIntValue, 1));
    gUint8[1] = usat8(vgetq_lane_s32(gIntValue, 1));
    bUint8[1] = usat8(vgetq_lane_s32(bIntValue, 1));

    rUint8[2] = usat8(vgetq_lane_s32(rIntValue, 2));
    gUint8[2] = usat8(vgetq_lane_s32(gIntValue, 2));
    bUint8[2] = usat8(vgetq_lane_s32(bIntValue, 2));

    rUint8[3] = usat8(vgetq_lane_s32(rIntValue, 3));
    gUint8[3] = usat8(vgetq_lane_s32(gIntValue, 3));
    bUint8[3] = usat8(vgetq_lane_s32(bIntValue, 3));
}

static void yuv2rgb_bt709V(vx_float32* y, vx_float32* cb, vx_float32* cr,
    vx_uint8 *rUint8, vx_uint8 *gUint8, vx_uint8 *bUint8)
{
    float32x4_t y32X4Value = vld1q_f32(y);
    float32x4_t cb32X4Value = vld1q_f32(cb);
    float32x4_t cr32X4Value = vld1q_f32(cr);
    float32x4_t All128 = vdupq_n_f32(128.0f);
    float32x4_t AllZero = vdupq_n_f32(0.0f);
    float32x4_t rFloatValue, gFloatValue, bFloatValue;
    int32x4_t  rIntValue, gIntValue, bIntValue;
    cb32X4Value = vsubq_f32(cb32X4Value,All128);
    cr32X4Value = vsubq_f32(cr32X4Value,All128);

    // R'= Y' + 0.000*U' + 1.403*V'
    // G'= Y' - 0.344*U' - 0.714*V'
    // B'= Y' + 1.773*U' + 0.000*V'
    rFloatValue = vmlaq_n_f32(y32X4Value, cr32X4Value, 1.5748f);

    gFloatValue = vmlaq_n_f32(y32X4Value, cb32X4Value, -0.1873f);
    gFloatValue = vmlaq_n_f32(gFloatValue, cr32X4Value, -0.4681f);

    bFloatValue = vmlaq_n_f32(y32X4Value, cb32X4Value, 1.8556f);

    rIntValue = vcvtq_s32_f32(rFloatValue);
    gIntValue = vcvtq_s32_f32(gFloatValue);
    bIntValue = vcvtq_s32_f32(bFloatValue);

    rUint8[0] = usat8(vgetq_lane_s32(rIntValue, 0));
    gUint8[0] = usat8(vgetq_lane_s32(gIntValue, 0));
    bUint8[0] = usat8(vgetq_lane_s32(bIntValue, 0));

    rUint8[1] = usat8(vgetq_lane_s32(rIntValue, 1));
    gUint8[1] = usat8(vgetq_lane_s32(gIntValue, 1));
    bUint8[1] = usat8(vgetq_lane_s32(bIntValue, 1));

    rUint8[2] = usat8(vgetq_lane_s32(rIntValue, 2));
    gUint8[2] = usat8(vgetq_lane_s32(gIntValue, 2));
    bUint8[2] = usat8(vgetq_lane_s32(bIntValue, 2));

    rUint8[3] = usat8(vgetq_lane_s32(rIntValue, 3));
    gUint8[3] = usat8(vgetq_lane_s32(gIntValue, 3));
    bUint8[3] = usat8(vgetq_lane_s32(bIntValue, 3));
}

// kernel --------------------------------------------------------------------

// nodeless version of the ConvertColor kernel
vx_status vxConvertColor(vx_image src, vx_image dst)
{
    vx_imagepatch_addressing_t src_addr[4], dst_addr[4];
    void *src_base[4] = {NULL};
    void *dst_base[4] = {NULL};
    vx_uint32 y, x, p;
    vx_df_image src_format, dst_format;
    vx_size src_planes, dst_planes;
    vx_enum src_space;
    vx_rectangle_t rect;

    vx_status status = VX_SUCCESS;
    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    status |= vxQueryImage(src, VX_IMAGE_PLANES, &src_planes, sizeof(src_planes));
    status |= vxQueryImage(dst, VX_IMAGE_PLANES, &dst_planes, sizeof(dst_planes));
    status |= vxQueryImage(src, VX_IMAGE_SPACE, &src_space, sizeof(src_space));
    status = vxGetValidRegionImage(src, &rect);

    vx_map_id map_id[4] = {0, 0, 0, 0};
    vx_map_id result_map_id[4] = {0, 0, 0, 0};
    for (p = 0; p < src_planes; p++)
    {
        status |= vxMapImagePatch(src, &rect, p, &map_id[p], &src_addr[p], &src_base[p], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        ownPrintImageAddressing(&src_addr[p]);
    }
    for (p = 0; p < dst_planes; p++)
    {
        status |= vxMapImagePatch(dst, &rect, p, &result_map_id[p], &dst_addr[p], &dst_base[p], VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
        ownPrintImageAddressing(&dst_addr[p]);
    }
    if (status != VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to setup images in Color Convert!\n");
    }

    vx_uint32 width = dst_addr[0].dim_x;
    vx_uint32 height = dst_addr[0].dim_y;

    vx_uint8 *srcP0 = (vx_uint8 *)src_base[0];
    vx_uint8 *dstP0 = (vx_uint8 *)dst_base[0];

    vx_uint8 *srcP1 = (vx_uint8 *)src_base[1];
    vx_uint8 *dstP1 = (vx_uint8 *)dst_base[1];

    vx_uint8 *srcP2 = (vx_uint8 *)src_base[2];
    vx_uint8 *dstP2 = (vx_uint8 *)dst_base[2];

    vx_uint32 srcP0StrideX = src_addr[0].stride_x;
    vx_uint32 srcP0StrideY = src_addr[0].stride_y;
    vx_uint32 dstP0StrideX = dst_addr[0].stride_x;
    vx_uint32 dstP0StrideY = dst_addr[0].stride_y;

    vx_uint32 srcP1StrideX = src_addr[1].stride_x;
    vx_uint32 srcP1StrideY = src_addr[1].stride_y;
    vx_uint32 dstP1StrideX = dst_addr[1].stride_x;
    vx_uint32 dstP1StrideY = dst_addr[1].stride_y;

    vx_uint32 srcP2StrideX = src_addr[2].stride_x;
    vx_uint32 srcP2StrideY = src_addr[2].stride_y;
    vx_uint32 dstP2StrideX = dst_addr[2].stride_x;
    vx_uint32 dstP2StrideY = dst_addr[2].stride_y;

    if ((src_format == VX_DF_IMAGE_RGB) || (src_format == VX_DF_IMAGE_RGBX))
    {
        if (dst_format == VX_DF_IMAGE_RGB || dst_format == VX_DF_IMAGE_RGBX)
        {
            vx_uint32 imgSize = dst_addr[0].dim_y * dst_addr[0].dim_x;

            if (dst_format == VX_DF_IMAGE_RGB)
            {
                for (x = 0; x < imgSize; x += 8)
                {
                    uint8x8x4_t s = vld4_u8(srcP0);

                    uint8x8x3_t d;
                    d.val[0] = s.val[0];
                    d.val[1] = s.val[1];
                    d.val[2] = s.val[2];

                    vst3_u8(dstP0, d); 

                    srcP0 += srcP0StrideX * 8; 
                    dstP0 += dstP0StrideX * 8;
                }
            }
            else
            {
                for (x = 0; x < imgSize; x += 8)
                { 
                    uint8x8x3_t s = vld3_u8(srcP0);

                    uint8x8x4_t d;
                    d.val[0] = s.val[0];
                    d.val[1] = s.val[1];
                    d.val[2] = s.val[2];
                    d.val[3] = vdup_n_u8(255);

                    vst4_u8(dstP0, d); 

                    srcP0 += srcP0StrideX * 8; 
                    dstP0 += dstP0StrideX * 8;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            vx_uint8 cb[4];
            vx_uint8 cr[4];
            vx_uint8 *rgb[4];
            vx_uint8 *luma[4]; 
            vx_uint8 *cbcr;

            for (y = 0; y < height; y += 2)
            {
                for (x = 0; x < width; x += 2)
                {
                    rgb[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    rgb[1] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+1) * srcP0StrideX;
                    rgb[2] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + x * srcP0StrideX;
                    rgb[3] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + (x+1) * srcP0StrideX;

                    luma[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    luma[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    luma[2] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + x * dstP0StrideX;
                    luma[3] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + (x+1) * dstP0StrideX;

                    cbcr = (vx_uint8 *)dst_base[1] + y * dstP1StrideY / 2 + x * dstP1StrideX / 2;
           
                    vx_float32 arrfr[4] = { (vx_float32)rgb[0][0], (vx_float32)rgb[1][0], (vx_float32)rgb[2][0], (vx_float32)rgb[3][0] };
                    vx_float32 arrfg[4] = { (vx_float32)rgb[0][1], (vx_float32)rgb[1][1], (vx_float32)rgb[2][1], (vx_float32)rgb[3][1] };
                    vx_float32 arrfb[4] = { (vx_float32)rgb[0][2], (vx_float32)rgb[1][2], (vx_float32)rgb[2][2], (vx_float32)rgb[3][2] };

                    rgb2yuv_bt709_neon(arrfr, arrfg, arrfb, luma, &cb[0], &cr[0]);

                    cbcr[0] = (cb[0] + cb[1] + cb[2] + cb[3]) / 4;
                    cbcr[1] = (cr[0] + cr[1] + cr[2] + cr[3]) / 4;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            vx_uint8 cb[4];
            vx_uint8 cr[4];
            vx_uint8 *rgb[4];
            vx_uint8 *luma[4]; 
            vx_uint8 *u[4];
            vx_uint8 *v[4];
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x += 4)
                {
                    rgb[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    rgb[1] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+1) * srcP0StrideX;
                    rgb[2] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+2) * srcP0StrideX;
                    rgb[3] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+3) * srcP0StrideX;

                    luma[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    luma[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    luma[2] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+2) * dstP0StrideX;
                    luma[3] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+3) * dstP0StrideX;

                    u[0] = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + x * dstP1StrideX;
                    u[1] = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + (x+1) * dstP1StrideX;
                    u[2] = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + (x+2) * dstP1StrideX;
                    u[3] = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + (x+3) * dstP1StrideX;

                    v[0] = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + x * dstP2StrideX;
                    v[1] = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + (x+1) * dstP2StrideX;
                    v[2] = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + (x+2) * dstP2StrideX;
                    v[3] = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + (x+3) * dstP2StrideX;

                    vx_float32 arrfr[4] = { (vx_float32)rgb[0][0], (vx_float32)rgb[1][0], (vx_float32)rgb[2][0], (vx_float32)rgb[3][0] };
                    vx_float32 arrfg[4] = { (vx_float32)rgb[0][1], (vx_float32)rgb[1][1], (vx_float32)rgb[2][1], (vx_float32)rgb[3][1] };
                    vx_float32 arrfb[4] = { (vx_float32)rgb[0][2], (vx_float32)rgb[1][2], (vx_float32)rgb[2][2], (vx_float32)rgb[3][2] };

                    rgb2yuv_bt709_neon(arrfr, arrfg, arrfb, luma, &cb[0], &cr[0]);

                    *u[0] = cb[0];
                    *u[1] = cb[1]; 
                    *u[2] = cb[2]; 
                    *u[3] = cb[3];

                    *v[0] = cr[0];
                    *v[1] = cr[1]; 
                    *v[2] = cr[2]; 
                    *v[3] = cr[3];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            vx_uint8 cb[4];
            vx_uint8 cr[4];
            vx_uint8 *rgb[4];
            vx_uint8 *luma[4]; 
            vx_uint8 *cbp;
            vx_uint8 *crp;
            for (y = 0; y < height; y += 2)
            {
                for (x = 0; x < width; x += 2)
                {
                    rgb[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    rgb[1] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+1) * srcP0StrideX;
                    rgb[2] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + x * srcP0StrideX;
                    rgb[3] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + (x+1) * srcP0StrideX;

                    luma[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    luma[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    luma[2] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + x * dstP0StrideX;
                    luma[3] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + (x+1) * dstP0StrideX;

                    cbp = (vx_uint8 *)dst_base[1] + y * dstP1StrideY / 2 + x * dstP1StrideX / 2;
                    crp = (vx_uint8 *)dst_base[2] + y * dstP2StrideY / 2 + x * dstP2StrideX / 2;

                    vx_float32 arrfr[4] = { (vx_float32)rgb[0][0], (vx_float32)rgb[1][0], (vx_float32)rgb[2][0], (vx_float32)rgb[3][0] };
                    vx_float32 arrfg[4] = { (vx_float32)rgb[0][1], (vx_float32)rgb[1][1], (vx_float32)rgb[2][1], (vx_float32)rgb[3][1] };
                    vx_float32 arrfb[4] = { (vx_float32)rgb[0][2], (vx_float32)rgb[1][2], (vx_float32)rgb[2][2], (vx_float32)rgb[3][2] };

                    rgb2yuv_bt709_neon(arrfr, arrfg, arrfb, luma, &cb[0], &cr[0]);

                    cbp[0] = (vx_uint8)(((vx_uint16)cb[0] + (vx_uint16)cb[1] + (vx_uint16)cb[2] + (vx_uint16)cb[3]) >> 2);
                    crp[0] = (vx_uint8)(((vx_uint16)cr[0] + (vx_uint16)cr[1] + (vx_uint16)cr[2] + (vx_uint16)cr[3]) >> 2);
                }
            }

        }
    }
    else if (src_format == VX_DF_IMAGE_NV21 || src_format == VX_DF_IMAGE_NV12)
    {
        int u_pix = src_format == VX_DF_IMAGE_NV12 ? 0 : 1;
        int v_pix = src_format == VX_DF_IMAGE_NV12 ? 1 : 0;
        if ((dst_format == VX_DF_IMAGE_RGB) || (dst_format == VX_DF_IMAGE_RGBX))
        {
            vx_uint8 *rgb[4];
            vx_uint8 *luma[4]; 
            vx_uint8 *crcb;
            for (y = 0; y < height; y += 2)
            {
                for (x = 0; x < width; x += 2)
                {
                    luma[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    luma[1] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+1) * srcP0StrideX;
                    luma[2] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + x * srcP0StrideX;
                    luma[3] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + (x+1) * srcP0StrideX;

                    crcb = (vx_uint8 *)src_base[1] + y * srcP1StrideY / 2 + x * srcP1StrideX / 2;

                    rgb[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    rgb[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    rgb[2] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + x * dstP0StrideX;
                    rgb[3] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + (x+1) * dstP0StrideX;

                    if (dst_format == VX_DF_IMAGE_RGBX)
                    {
                        rgb[0][3] = 255;
                        rgb[1][3] = 255;
                        rgb[2][3] = 255;
                        rgb[3][3] = 255;

                    } 
                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601_neon(luma, crcb[u_pix], crcb[v_pix], rgb, rgb, rgb);
                    }
                    else 
                    {
                        yuv2rgb_bt709_neon(luma, crcb[u_pix], crcb[v_pix], rgb, rgb, rgb);
                    }
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12 || dst_format == VX_DF_IMAGE_NV21)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0])};
                    vx_uint8 *cbcr = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *crcb = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    yuv2yuv_601to709(luma[0][0],cbcr[0],cbcr[1],&luma[1][0],&crcb[1],&crcb[0]);
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x+=8)
                {
                    uint8x8_t lumaV8 = vld1_u8(srcP0); 
                    vst1_u8(dstP0, lumaV8);

                    srcP0 += srcP0StrideX * 8; 
                    dstP0 += dstP0StrideX * 8; 
                }
            }

            vx_uint8 *crcb = NULL;
            vx_uint8 *cb[4] = { NULL };
            vx_uint8 *cr[4] = { NULL };
            for (y = 0; y < height; y+=2)
            {
                for (x = 0; x < width; x+=2)
                {
                    crcb = (vx_uint8 *)src_base[1] + y * srcP1StrideY / 2 + x * srcP1StrideX / 2;

                    cb[0] = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + x * dstP1StrideX;
                    cb[1] = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + (x+1) * dstP1StrideX;
                    cb[2] = (vx_uint8 *)dst_base[1] + (y+1) * dstP1StrideY + x * dstP1StrideX;
                    cb[3] = (vx_uint8 *)dst_base[1] + (y+1) * dstP1StrideY + (x+1) * dstP1StrideX;

                    cr[0] = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + x * dstP2StrideX;
                    cr[1] = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + (x+1) * dstP2StrideX;
                    cr[2] = (vx_uint8 *)dst_base[2] + (y+1) * dstP2StrideY + x * dstP2StrideX;
                    cr[3] = (vx_uint8 *)dst_base[2] + (y+1) * dstP2StrideY + (x+1) * dstP2StrideX;

                    cb[0][0] = crcb[u_pix];
                    cb[1][0] = crcb[u_pix];
                    cb[2][0] = crcb[u_pix];
                    cb[3][0] = crcb[u_pix];

                    cr[0][0] = crcb[v_pix];
                    cr[1][0] = crcb[v_pix];
                    cr[2][0] = crcb[v_pix];
                    cr[3][0] = crcb[v_pix];

                }
            }    
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x+=8)
                {
                    uint8x8_t lumaV8 = vld1_u8(srcP0); 
                    vst1_u8(dstP0, lumaV8);

                    srcP0 += srcP0StrideX * 8; 
                    dstP0 += dstP0StrideX * 8; 
                }
            }

            vx_uint32 halfwidth = width/2; 
            vx_uint32 halfheight = height/2;
            if(halfwidth % 8)
            {
                vx_uint8 *crcb[4];
                vx_uint8 *cb[4];
                vx_uint8 *cr[4];
                for (y = 0; y < height; y++)
                {
                    for (x = 0; x < width; x += 4)
                    {
                        for(int i = 0; i < 4; i++)
                        {
                            crcb[i]= vxFormatImagePatchAddress2d(src_base[1], x+i, y, &src_addr[1]);
                            cb[i]= vxFormatImagePatchAddress2d(dst_base[1], x+i, y, &dst_addr[1]);
                            cr[i]= vxFormatImagePatchAddress2d(dst_base[2], x+i, y, &dst_addr[2]);

                            cb[i][0] = crcb[i][u_pix];
                            cr[i][0] = crcb[i][v_pix];   
                        }
                    }
                }
            }
            else
            {
                vx_uint8 *sP1 = srcP1;
                vx_uint8 *dP1 = dstP1; 
                vx_uint8 *dP2 = dstP2; 
                for (y = 0; y < halfheight; y++)
                {
                    for (x = 0; x < halfwidth; x += 8)
                    {
                        uint8x8x2_t s = vld2_u8(sP1);

                        vst1_u8(dP1, s.val[u_pix]); 
                        vst1_u8(dP2, s.val[v_pix]); 

                        sP1 += srcP1StrideX * 8; 
                        dP1 += dstP1StrideX * 8; 
                        dP2 += dstP2StrideX * 8; 
                    }
                }
            }
        }
    }
    else if (src_format == VX_DF_IMAGE_YUYV)
    {
        if (dst_format == VX_DF_IMAGE_RGB || dst_format == VX_DF_IMAGE_RGBX)
        {
            vx_uint32 x, y;
            vx_uint32 width4 = width / 4 * 4;
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width4; x += 4)
                {
                    vx_uint8 *yuyv = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    vx_uint8 *yuyv1 = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+2) * srcP0StrideX;

                    vx_float32 yValue[4] = {(vx_float32)yuyv[0],(vx_float32)yuyv[2],(vx_float32)yuyv1[0],(vx_float32)yuyv1[2]};
                    vx_float32 cbValue[4] = {(vx_float32)yuyv[1],(vx_float32)yuyv[1],(vx_float32)yuyv1[1],(vx_float32)yuyv1[1]};
                    vx_float32 crValue[4] = {(vx_float32)yuyv[3],(vx_float32)yuyv[3],(vx_float32)yuyv1[3],(vx_float32)yuyv1[3]};

                    vx_uint8 *rgb0 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    vx_uint8 *rgb1 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    vx_uint8 *rgb2 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+2) * dstP0StrideX;
                    vx_uint8 *rgb3 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+3) * dstP0StrideX;

                    vx_uint8 bUint8[4];
                    vx_uint8 gUint8[4];
                    vx_uint8 rUint8[4];

                    if(src_space == VX_COLOR_SPACE_BT601_525 || src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601V(yValue, cbValue, crValue, rUint8, gUint8, bUint8);

                        rgb0[0] = rUint8[0];
                        rgb1[0] = rUint8[1];
                        rgb2[0] = rUint8[2];
                        rgb3[0] = rUint8[3];

                        rgb0[1] = gUint8[0];
                        rgb1[1] = gUint8[1];
                        rgb2[1] = gUint8[2];
                        rgb3[1] = gUint8[3];

                        rgb0[2] = bUint8[0];
                        rgb1[2] = bUint8[1];
                        rgb2[2] = bUint8[2];
                        rgb3[2] = bUint8[3];
                        if (dst_format == VX_DF_IMAGE_RGBX)
                        {
                            rgb0[3] = 255;
                            rgb1[3] = 255;
                            rgb2[3] = 255;
                            rgb3[3] = 255;
                        }
                    }
                    else
                    {
                        yuv2rgb_bt709V(yValue, cbValue, crValue, rUint8, gUint8, bUint8);

                        rgb0[0] = rUint8[0];
                        rgb1[0] = rUint8[1];
                        rgb2[0] = rUint8[2];
                        rgb3[0] = rUint8[3];

                        rgb0[1] = gUint8[0];
                        rgb1[1] = gUint8[1];
                        rgb2[1] = gUint8[2];
                        rgb3[1] = gUint8[3];

                        rgb0[2] = bUint8[0];
                        rgb1[2] = bUint8[1];
                        rgb2[2] = bUint8[2];
                        rgb3[2] = bUint8[3];
                        if (dst_format == VX_DF_IMAGE_RGBX)
                        {
                            rgb0[3] = 255;
                            rgb1[3] = 255;
                            rgb2[3] = 255;
                            rgb3[3] = 255;
                        }
                    }
                }
                if (dst_format == VX_DF_IMAGE_RGB)
                {
                    for (; x < width; x += 2)
                    {
                        vx_uint8 *yuyv = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                        vx_uint8 *rgb = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;

                        if (src_space == VX_COLOR_SPACE_BT601_525 ||
                            src_space == VX_COLOR_SPACE_BT601_625)
                        {
                            yuv2rgb_bt601(yuyv[0], yuyv[1], yuyv[3], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt601(yuyv[2], yuyv[1], yuyv[3], &rgb[3], &rgb[4], &rgb[5]);
                        }
                        else
                        {
                            yuv2rgb_bt709(yuyv[0], yuyv[1], yuyv[3], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt709(yuyv[2], yuyv[1], yuyv[3], &rgb[3], &rgb[4], &rgb[5]);
                        }
                    }
                }
                else if (dst_format == VX_DF_IMAGE_RGBX)
                {
                    for (; x < width; x += 2)
                    {
                        vx_uint8 *yuyv = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                        vx_uint8 *rgb = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                        rgb[3] = rgb[7] = 255;

                        if (src_space == VX_COLOR_SPACE_BT601_525 ||
                            src_space == VX_COLOR_SPACE_BT601_625)
                        {
                            yuv2rgb_bt601(yuyv[0], yuyv[1], yuyv[3], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt601(yuyv[2], yuyv[1], yuyv[3], &rgb[4], &rgb[5], &rgb[6]);
                        }
                        else
                        {
                            yuv2rgb_bt709(yuyv[0], yuyv[1], yuyv[3], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt709(yuyv[2], yuyv[1], yuyv[3], &rgb[4], &rgb[5], &rgb[6]);
                        }
                    }
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            vx_uint32 x, y;
            vx_uint32 width8 = width / 8 * 8;
            vx_uint8 *yuyv[2];
            vx_uint8 *luma[4];
            for (y = 0; y < height; y += 2)
            {
                vx_uint8 *src0 = srcP0 + y * srcP0StrideY;
                vx_uint8 *src1 = srcP0 + (y + 1) * srcP0StrideY;
                vx_uint8 *dstLuma = dstP0 + y * dstP0StrideY;
                vx_uint8 *dstLuma1 = dstP0 + (y + 1) * dstP0StrideY;
                vx_uint8 *dstCbCr = dstP1 + (y >> 1) * dstP0StrideY;
                for (x = 0; x < width8; x += 8)
                {
                    uint8x8_t srcValue00 = vld1_u8(src0 + x * srcP0StrideX);
                    uint8x8_t srcValue01 = vld1_u8(src0 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dstValue0 = vuzp_u8(srcValue00, srcValue01);
                    vst1_u8((dstLuma + x * dstP0StrideX),dstValue0.val[0]);

                    uint8x8_t srcValue10 = vld1_u8(src1 + x * srcP0StrideX);
                    uint8x8_t srcValue11 = vld1_u8(src1 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dstValue1 = vuzp_u8(srcValue10, srcValue11);
                    vst1_u8((dstLuma1 + x * dstP0StrideX),dstValue1.val[0]);

                    uint16x8_t cbcrValue = vaddl_u8(dstValue0.val[1],dstValue1.val[1]);

                    vx_uint16 cbcrValuek[8];
                    vst1q_u16(cbcrValuek,cbcrValue);
                    for (vx_uint32 kx = 0; kx < 8; kx += 2)
                    {
                        *(dstCbCr + ((x + kx) >> 1) * dstP1StrideX) = cbcrValuek[kx] / 2;
                        *(dstCbCr + ((x + kx) >> 1) * dstP1StrideX + 1) = cbcrValuek[kx + 1] / 2;
                    }

                }
                for (; x < width; x += 2)
                {
                    yuyv[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    yuyv[1] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + x * srcP0StrideX;

                    luma[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    luma[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    luma[2] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + x * dstP0StrideX;
                    luma[3] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + (x+1) * dstP0StrideX;

                    vx_uint8 *cbcr = (vx_uint8 *)dst_base[1] + y * dstP1StrideY / 2 + x * dstP1StrideX / 2;

                    luma[0][0] = yuyv[0][0];
                    luma[1][0] = yuyv[0][2];
                    luma[2][0] = yuyv[1][0];
                    luma[3][0] = yuyv[1][2];
                    cbcr[0] = (yuyv[0][1] + yuyv[1][1]) / 2;
                    cbcr[1] = (yuyv[0][3] + yuyv[1][3]) / 2;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *yuyv = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    vx_uint8 *luma = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    vx_uint8 *cb = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + x * dstP1StrideX;
                    vx_uint8 *cr = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + x * dstP2StrideX;

                    luma[0] = yuyv[0];
                    luma[1] = yuyv[2];
                    cb[0] = yuyv[1];
                    cr[0] = yuyv[3];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            vx_uint32 x, y;
            vx_uint32 width8 = width / 8 * 8;
            vx_uint8 *yuyv[2];
            vx_uint8 *_luma[4];
            for (y = 0; y < height; y += 2)
            {
                vx_uint8 *src0 = srcP0 + y * srcP0StrideY;
                vx_uint8 *src1 = srcP0 + (y + 1) * srcP0StrideY; 
                vx_uint8 *luma = dstP0 + y * dstP0StrideY;
                vx_uint8 *luma1 = dstP0 + (y + 1) * dstP0StrideY;
                vx_uint8 *cb = dstP1 + (y >> 1) * dstP1StrideY;
                vx_uint8 *cr = dstP2 + (y >> 1) * dstP2StrideY;

                for (x = 0; x < width8; x += 8)
                {
                    uint8x8_t src00Value = vld1_u8(src0 + x * srcP0StrideX);
                    uint8x8_t src01Value = vld1_u8(src0 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dst0Value = vuzp_u8(src00Value,src01Value);
                    vst1_u8((luma + x * dstP0StrideX),dst0Value.val[0]);

                    uint8x8_t src10Value = vld1_u8(src1 + x * srcP0StrideX);
                    uint8x8_t src11Value = vld1_u8(src1 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dst1Value = vuzp_u8(src10Value,src11Value);
                    vst1_u8((luma1 + x * dstP0StrideX),dst1Value.val[0]);

                    uint16x8_t cbcrValue = vaddl_u8(dst0Value.val[1],dst1Value.val[1]);
                    vx_uint16 cbcrValuek[8];
                    vst1q_u16(cbcrValuek,cbcrValue);
                    for (vx_uint32 kx = 0; kx < 8; kx += 2)
                    {
                        *(cb + ((x + kx) >> 1) * dstP1StrideX) = cbcrValuek[kx] / 2;
                        *(cr + ((x + kx) >> 1) * dstP2StrideX) = cbcrValuek[kx + 1] / 2;
                    }
                }
                for(; x < width; x += 2)
                {
                    yuyv[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    yuyv[1] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + x * srcP0StrideX;

                    _luma[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    _luma[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    _luma[2] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + x * dstP0StrideX;
                    _luma[3] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + (x+1) * dstP0StrideX;

                    vx_uint8 *cb = (vx_uint8 *)dst_base[1] + y * dstP1StrideY / 2 + x * dstP1StrideX / 2;
                    vx_uint8 *cr = (vx_uint8 *)dst_base[2] + y * dstP2StrideY / 2 + x * dstP2StrideX / 2;

                    _luma[0][0] = yuyv[0][0];
                    _luma[1][0] = yuyv[0][2];
                    _luma[2][0] = yuyv[1][0];
                    _luma[3][0] = yuyv[1][2];
                    cb[0] = (yuyv[0][1] + yuyv[1][1]) / 2;
                    cr[0] = (yuyv[0][3] + yuyv[1][3]) / 2;
                }
            }
        }
    }
    else if (src_format == VX_DF_IMAGE_UYVY)
    {
        if (dst_format == VX_DF_IMAGE_RGB || dst_format == VX_DF_IMAGE_RGBX)
        {
            vx_uint32 x, y;
            vx_uint32 width4 = width / 4 * 4;
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width4; x += 4)
                {
                    vx_uint8 *uyvy = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    vx_uint8 *uyvy1 = (vx_uint8 *)src_base[0] + y * srcP0StrideY + (x+2) * srcP0StrideX;

                    vx_float32 yValue[4] = {(vx_float32)uyvy[1],(vx_float32)uyvy[3],(vx_float32)uyvy1[1],(vx_float32)uyvy1[3]};
                    vx_float32 cbValue[4] = {(vx_float32)uyvy[0],(vx_float32)uyvy[0],(vx_float32)uyvy1[0],(vx_float32)uyvy1[0]};
                    vx_float32 crValue[4] = {(vx_float32)uyvy[2],(vx_float32)uyvy[2],(vx_float32)uyvy1[2],(vx_float32)uyvy1[2]};

                    vx_uint8 *rgb0 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    vx_uint8 *rgb1 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    vx_uint8 *rgb2 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+2) * dstP0StrideX;
                    vx_uint8 *rgb3 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+3) * dstP0StrideX;

                    vx_uint8 bUint8[4];
                    vx_uint8 gUint8[4];
                    vx_uint8 rUint8[4];

                    if(src_space == VX_COLOR_SPACE_BT601_525 || src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601V(yValue, cbValue, crValue, rUint8, gUint8, bUint8);

                        rgb0[0] = rUint8[0];
                        rgb1[0] = rUint8[1];
                        rgb2[0] = rUint8[2];
                        rgb3[0] = rUint8[3];

                        rgb0[1] = gUint8[0];
                        rgb1[1] = gUint8[1];
                        rgb2[1] = gUint8[2];
                        rgb3[1] = gUint8[3];

                        rgb0[2] = bUint8[0];
                        rgb1[2] = bUint8[1];
                        rgb2[2] = bUint8[2];
                        rgb3[2] = bUint8[3];
                        if (dst_format == VX_DF_IMAGE_RGBX)
                        {
                            rgb0[3] = 255;
                            rgb1[3] = 255;
                            rgb2[3] = 255;
                            rgb3[3] = 255;
                        }
                    }
                    else
                    {
                        yuv2rgb_bt709V(yValue, cbValue, crValue, rUint8, gUint8, bUint8);

                        rgb0[0] = rUint8[0];
                        rgb1[0] = rUint8[1];
                        rgb2[0] = rUint8[2];
                        rgb3[0] = rUint8[3];

                        rgb0[1] = gUint8[0];
                        rgb1[1] = gUint8[1];
                        rgb2[1] = gUint8[2];
                        rgb3[1] = gUint8[3];

                        rgb0[2] = bUint8[0];
                        rgb1[2] = bUint8[1];
                        rgb2[2] = bUint8[2];
                        rgb3[2] = bUint8[3];
                        if (dst_format == VX_DF_IMAGE_RGBX)
                        {
                            rgb0[3] = 255;
                            rgb1[3] = 255;
                            rgb2[3] = 255;
                            rgb3[3] = 255;
                        }
                    }

                }
                if (dst_format == VX_DF_IMAGE_RGB)
                {
                    for (; x < width; x += 2)
                    {
                        vx_uint8 *uyvy = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                        vx_uint8 *rgb = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;

                        if (src_space == VX_COLOR_SPACE_BT601_525 ||
                            src_space == VX_COLOR_SPACE_BT601_625)
                        {
                            yuv2rgb_bt601(uyvy[1], uyvy[0], uyvy[2], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt601(uyvy[3], uyvy[0], uyvy[2], &rgb[3], &rgb[4], &rgb[5]);
                        }
                        else
                        {
                            yuv2rgb_bt709(uyvy[1], uyvy[0], uyvy[2], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt709(uyvy[3], uyvy[0], uyvy[2], &rgb[3], &rgb[4], &rgb[5]);
                        }
                    }
                }
                else if (dst_format == VX_DF_IMAGE_RGBX)
                {
                    for (; x < width; x += 2)
                    {
                        vx_uint8 *uyvy = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                        vx_uint8 *rgb = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                        rgb[3] = rgb[7] = 255;

                        if (src_space == VX_COLOR_SPACE_BT601_525 ||
                            src_space == VX_COLOR_SPACE_BT601_625)
                        {
                            yuv2rgb_bt601(uyvy[1], uyvy[0], uyvy[2], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt601(uyvy[3], uyvy[0], uyvy[2], &rgb[4], &rgb[5], &rgb[6]);
                        }
                        else
                        {
                            yuv2rgb_bt709(uyvy[1], uyvy[0], uyvy[2], &rgb[0], &rgb[1], &rgb[2]);
                            yuv2rgb_bt709(uyvy[3], uyvy[0], uyvy[2], &rgb[4], &rgb[5], &rgb[6]);
                        }
                    }
                }
            } 
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            vx_uint32 x, y;
            vx_uint32 width8 = width / 8 * 8;
            vx_uint8 *uyvy[2];
            vx_uint8 *luma[4];
            for (y = 0; y < height; y += 2)
            {
                vx_uint8 *src0 = srcP0 + y * srcP0StrideY;
                vx_uint8 *src1 = srcP0 + (y + 1) * srcP0StrideY;
                vx_uint8 *dstLuma = dstP0 + y * dstP0StrideY;
                vx_uint8 *dstLuma1 = dstP0 + (y + 1) * dstP0StrideY;
                vx_uint8 *dstCbCr = dstP1 + (y >> 1) * dstP0StrideY;
                for (x = 0; x < width8; x += 8)
                {
                    uint8x8_t srcValue00 = vld1_u8(src0 + x * srcP0StrideX);
                    uint8x8_t srcValue01 = vld1_u8(src0 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dstValue0 = vuzp_u8(srcValue00, srcValue01);
                    vst1_u8((dstLuma + x * dstP0StrideX),dstValue0.val[1]);

                    uint8x8_t srcValue10 = vld1_u8(src1 + x * srcP0StrideX);
                    uint8x8_t srcValue11 = vld1_u8(src1 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dstValue1 = vuzp_u8(srcValue10, srcValue11);
                    vst1_u8((dstLuma1 + x * dstP0StrideX),dstValue1.val[1]);

                    uint16x8_t cbcrValue = vaddl_u8(dstValue0.val[0],dstValue1.val[0]);

                    vx_uint16 cbcrValuek[8];
                    vst1q_u16(cbcrValuek,cbcrValue);
                    for (vx_uint32 kx = 0; kx < 8; kx += 2)
                    {
                        *(dstCbCr + ((x + kx) >> 1) * dstP1StrideX) = cbcrValuek[kx] / 2;
                        *(dstCbCr + ((x + kx) >> 1) * dstP1StrideX + 1) = cbcrValuek[kx + 1] / 2;
                    }

                }
                for (; x < width; x += 2)
                {
                    uyvy[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    uyvy[1] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + x * srcP0StrideX;

                    luma[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    luma[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    luma[2] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + x * dstP0StrideX;
                    luma[3] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + (x+1) * dstP0StrideX;

                    vx_uint8 *cbcr = (vx_uint8 *)dst_base[1] + y * dstP1StrideY / 2 + x * dstP1StrideX / 2;

                    luma[0][0] = uyvy[0][1];
                    luma[1][0] = uyvy[0][3];
                    luma[2][0] = uyvy[1][1];
                    luma[3][0] = uyvy[1][3];
                    cbcr[0] = (uyvy[0][0] + uyvy[1][0]) / 2;
                    cbcr[1] = (uyvy[0][2] + uyvy[1][2]) / 2;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *uyvy = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    vx_uint8 *luma = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    vx_uint8 *cb = (vx_uint8 *)dst_base[1] + y * dstP1StrideY + x * dstP1StrideX;
                    vx_uint8 *cr = (vx_uint8 *)dst_base[2] + y * dstP2StrideY + x * dstP2StrideX;

                    luma[0] = uyvy[1];
                    luma[1] = uyvy[3];
                    cb[0] = uyvy[0];
                    cr[0] = uyvy[2];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            vx_uint32 x, y;
            vx_uint32 width8 = width / 8 * 8;
            vx_uint8 *uyvy[2];
            vx_uint8 *_luma[4];
            for (y = 0; y < height; y += 2)
            {
                vx_uint8 *src0 = srcP0 + y * srcP0StrideY;
                vx_uint8 *src1 = srcP0 + (y + 1) * srcP0StrideY; 
                vx_uint8 *luma = dstP0 + y * dstP0StrideY;
                vx_uint8 *luma1 = dstP0 + (y + 1) * dstP0StrideY;
                vx_uint8 *cb = dstP1 + (y >> 1) * dstP1StrideY;
                vx_uint8 *cr = dstP2 + (y >> 1) * dstP2StrideY;

                for (x = 0; x < width8; x += 8)
                {
                    uint8x8_t src00Value = vld1_u8(src0 + x * srcP0StrideX);
                    uint8x8_t src01Value = vld1_u8(src0 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dst0Value = vuzp_u8(src00Value,src01Value);
                    vst1_u8((luma + x * dstP0StrideX),dst0Value.val[1]);

                    uint8x8_t src10Value = vld1_u8(src1 + x * srcP0StrideX);
                    uint8x8_t src11Value = vld1_u8(src1 + (x + 4) * srcP0StrideX);
                    uint8x8x2_t dst1Value = vuzp_u8(src10Value,src11Value);
                    vst1_u8((luma1 + x * dstP0StrideX),dst1Value.val[1]);

                    uint16x8_t cbcrValue = vaddl_u8(dst0Value.val[0], dst1Value.val[0]);
                    vx_uint16 cbcrValuek[8];
                    vst1q_u16(cbcrValuek, cbcrValue);
                    for (vx_uint32 kx = 0; kx < 8; kx += 2)
                    {
                        *(cb + ((x + kx) >> 1) * dstP1StrideX) = cbcrValuek[kx] / 2;
                        *(cr + ((x + kx) >> 1) * dstP2StrideX) = cbcrValuek[kx + 1] / 2;
                    }
                }
                for (; x < width; x += 2)
                {
                    uyvy[0] = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    uyvy[1] = (vx_uint8 *)src_base[0] + (y+1) * srcP0StrideY + x * srcP0StrideX;

                    _luma[0] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    _luma[1] = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    _luma[2] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + x * dstP0StrideX;
                    _luma[3] = (vx_uint8 *)dst_base[0] + (y+1) * dstP0StrideY + (x+1) * dstP0StrideX;

                    vx_uint8 *cb = (vx_uint8 *)dst_base[1] + y * dstP1StrideY / 2 + x * dstP1StrideX / 2;
                    vx_uint8 *cr = (vx_uint8 *)dst_base[2] + y * dstP2StrideY / 2 + x * dstP2StrideX / 2;

                    _luma[0][0] = uyvy[0][1];
                    _luma[1][0] = uyvy[0][3];
                    _luma[2][0] = uyvy[1][1];
                    _luma[3][0] = uyvy[1][3];
                    cb[0] = (uyvy[0][0] + uyvy[1][0]) / 2;
                    cr[0] = (uyvy[0][2] + uyvy[1][2]) / 2;
                }
            }
        }
    }
    else if (src_format == VX_DF_IMAGE_IYUV)
    {
        if (dst_format == VX_DF_IMAGE_RGB || dst_format == VX_DF_IMAGE_RGBX)
        {
            vx_uint32 x, y;
            vx_uint32 width4 = width / 4 * 4;
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width4; x += 4)
                {
                    vx_uint8 *luma = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    vx_uint8 *cb = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *cr = vxFormatImagePatchAddress2d(src_base[2], x, y, &src_addr[2]);

                    vx_float32 yValue[4] = {(vx_float32)luma[0],(vx_float32)luma[1],(vx_float32)luma[2],(vx_float32)luma[3]};
                    vx_float32 cbValue[4] = {(vx_float32)cb[0],(vx_float32)cb[0],(vx_float32)cb[1],(vx_float32)cb[1]};
                    vx_float32 crValue[4] = {(vx_float32)cr[0],(vx_float32)cr[0],(vx_float32)cr[1],(vx_float32)cr[1]};

                    vx_uint8 *rgb0 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;
                    vx_uint8 *rgb1 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+1) * dstP0StrideX;
                    vx_uint8 *rgb2 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+2) * dstP0StrideX;
                    vx_uint8 *rgb3 = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + (x+3) * dstP0StrideX;

                    vx_uint8 bUint8[4];
                    vx_uint8 gUint8[4];
                    vx_uint8 rUint8[4];

                    if (src_space == VX_COLOR_SPACE_BT601_525 || src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601V(yValue, cbValue, crValue, rUint8, gUint8, bUint8);

                        rgb0[0] = rUint8[0];
                        rgb1[0] = rUint8[1];
                        rgb2[0] = rUint8[2];
                        rgb3[0] = rUint8[3];

                        rgb0[1] = gUint8[0];
                        rgb1[1] = gUint8[1];
                        rgb2[1] = gUint8[2];
                        rgb3[1] = gUint8[3];

                        rgb0[2] = bUint8[0];
                        rgb1[2] = bUint8[1];
                        rgb2[2] = bUint8[2];
                        rgb3[2] = bUint8[3];
                        if (dst_format == VX_DF_IMAGE_RGBX)
                        {
                            rgb0[3] = 255;
                            rgb1[3] = 255;
                            rgb2[3] = 255;
                            rgb3[3] = 255;
                        }
                    }
                    else
                    {
                        yuv2rgb_bt709V(yValue, cbValue, crValue, rUint8, gUint8, bUint8);

                        rgb0[0] = rUint8[0];
                        rgb1[0] = rUint8[1];
                        rgb2[0] = rUint8[2];
                        rgb3[0] = rUint8[3];

                        rgb0[1] = gUint8[0];
                        rgb1[1] = gUint8[1];
                        rgb2[1] = gUint8[2];
                        rgb3[1] = gUint8[3];

                        rgb0[2] = bUint8[0];
                        rgb1[2] = bUint8[1];
                        rgb2[2] = bUint8[2];
                        rgb3[2] = bUint8[3];
                        if (dst_format == VX_DF_IMAGE_RGBX)
                        {
                            rgb0[3] = 255;
                            rgb1[3] = 255;
                            rgb2[3] = 255;
                            rgb3[3] = 255;
                        }
                    }
                }
                for (; x < width; x++)
                {
                    vx_uint8 *luma = (vx_uint8 *)src_base[0] + y * srcP0StrideY + x * srcP0StrideX;
                    vx_uint8 *cb = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *cr = vxFormatImagePatchAddress2d(src_base[2], x, y, &src_addr[2]);
                    vx_uint8 *rgb = (vx_uint8 *)dst_base[0] + y * dstP0StrideY + x * dstP0StrideX;

                    if (dst_format == VX_DF_IMAGE_RGBX)
                        rgb[3] = 255;

                    /*! \todo restricted range 601 ? */
                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                        yuv2rgb_bt601(luma[0], cb[0], cr[0], &rgb[0], &rgb[1], &rgb[2]);
                    else
                        yuv2rgb_bt709(luma[0], cb[0], cr[0], &rgb[0], &rgb[1], &rgb[2]);
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            vx_uint32 x, y;
            vx_uint32 width16 = width / 16 * 16;
            for (y = 0; y < height; y += 2)
            {
                vx_uint8 *luma = srcP0 + y * srcP0StrideY;
                vx_uint8 *luma1 = srcP0 + (y + 1) * srcP0StrideY;
                vx_uint8 *cb = srcP1 + (y >> 1) * srcP1StrideY;
                vx_uint8 *cr = srcP2 + (y >> 1) * srcP2StrideY;
                vx_uint8 *nv12Y = dstP0 + y * dstP0StrideY;
                vx_uint8 *nv12Y1 = dstP0 + (y + 1) * dstP0StrideY;
                vx_uint8 *nv12CbCr = dstP1 + (y >> 1) * dstP1StrideY;

                for (x = 0; x < width16; x += 16)
                {
                    uint8x16_t lumaValue = vld1q_u8(luma + x * srcP0StrideX);
                    vst1q_u8((nv12Y + x * dstP0StrideX), lumaValue);

                    uint8x16_t luma1Value = vld1q_u8(luma1 + x * srcP0StrideX);
                    vst1q_u8((nv12Y1 + x * dstP0StrideX), luma1Value);

                    uint8x8_t cbValue = vld1_u8(cb + (x >> 1) * srcP1StrideX);
                    uint8x8_t crValue = vld1_u8(cr + (x >> 1) * srcP2StrideX);

                    uint8x8x2_t cbcrValue = vzip_u8(cbValue, crValue);

                    vst1_u8((nv12CbCr + (x >> 1) * dstP1StrideX), cbcrValue.val[0]);
                    vst1_u8((nv12CbCr + ((x + 8) >> 1) * dstP1StrideX), cbcrValue.val[1]);
                }
                for (; x < width; x++)
                {
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(src_base[2], x, y, &src_addr[2]);
                    vx_uint8 *nv12[2] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1])};
                    nv12[0][0] = luma[0];
                    nv12[1][0] = cb[0];
                    nv12[1][1] = cr[0];
                    vx_uint8 *luma1 = vxFormatImagePatchAddress2d(src_base[0], x, (y + 1), &src_addr[0]);
                    vx_uint8 *nv12luma1 = vxFormatImagePatchAddress2d(dst_base[0], x, (y + 1), &dst_addr[0]);
                    nv12luma1[0] = luma1[0];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            vx_uint32 x, y;
            vx_uint32 width16 = width / 16 * 16;
            for (y = 0; y < height; y += 2)
            {
                vx_uint8 *luma = srcP0 + y * srcP0StrideY;
                vx_uint8 *luma1 = srcP0 + (y + 1) * srcP0StrideY;
                vx_uint8 *cb = srcP1 + (y >> 1) * srcP1StrideY;
                vx_uint8 *cr = srcP2 + (y >> 1) * srcP2StrideY;
                vx_uint8 *dstLuma = dstP0 + y * dstP0StrideY;
                vx_uint8 *dstLuma1 = dstP0 + (y + 1) * dstP0StrideY;
                vx_uint8 *dstcb = dstP1 + y * dstP1StrideY;
                vx_uint8 *dstcb1 = dstP1 + (y + 1) * dstP1StrideY;
                vx_uint8 *dstcr = dstP2 + y * dstP2StrideY;
                vx_uint8 *dstcr1 = dstP2 + (y + 1) * dstP1StrideY;
                for (x = 0; x < width16; x += 16)
                {
                    uint8x16_t lumaValue = vld1q_u8(luma + x * srcP0StrideX);
                    vst1q_u8((dstLuma + x * dstP0StrideX), lumaValue);

                    uint8x16_t luma1Value = vld1q_u8(luma1 + x * srcP0StrideX);
                    vst1q_u8((dstLuma1 + x * dstP0StrideX), luma1Value);

                    uint8x8_t cbValue = vld1_u8(cb + (x >> 1) * srcP1StrideX);
                    uint8x8x2_t dstCbValue = vzip_u8(cbValue, cbValue);
                    vst1_u8((dstcb + x * dstP1StrideX), dstCbValue.val[0]);
                    vst1_u8((dstcb + (x + 8) * dstP1StrideX), dstCbValue.val[1]);
                    vst1_u8((dstcb1 + x * dstP1StrideX), dstCbValue.val[0]);
                    vst1_u8((dstcb1 + (x + 8) * dstP1StrideX), dstCbValue.val[1]);

                    uint8x8_t crValue = vld1_u8(cr + (x >> 1) * srcP2StrideX);
                    uint8x8x2_t dstCrValue = vzip_u8(crValue, crValue);
                    vst1_u8((dstcr + x * dstP2StrideX), dstCrValue.val[0]);
                    vst1_u8((dstcr + (x + 8) * dstP2StrideX), dstCrValue.val[1]);
                    vst1_u8((dstcr1 + x * dstP2StrideX), dstCrValue.val[0]);
                    vst1_u8((dstcr1 + (x + 8) * dstP2StrideX), dstCrValue.val[1]);
                }
                for (; x < width; x++)
                {
                    vx_uint8 *luma[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0])};
                    vx_uint8 *cb[2]   = {vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]),
                                         vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1])};
                    vx_uint8 *cr[2]   = {vxFormatImagePatchAddress2d(src_base[2], x, y, &src_addr[2]),
                                         vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2])};

                    luma[1][0] = luma[0][0];
                    cb[1][0] = cb[0][0];
                    cr[1][0] = cr[0][0];

                    vx_uint8 *luma1[2] = { vxFormatImagePatchAddress2d(src_base[0], x, (y + 1), &src_addr[0]),
                        vxFormatImagePatchAddress2d(dst_base[0], x, (y + 1), &dst_addr[0]) };
                    vx_uint8 *cb1[2] = { vxFormatImagePatchAddress2d(src_base[1], x, (y + 1), &src_addr[1]),
                        vxFormatImagePatchAddress2d(dst_base[1], x, (y + 1), &dst_addr[1]) };
                    vx_uint8 *cr1[2] = { vxFormatImagePatchAddress2d(src_base[2], x, (y + 1), &src_addr[2]),
                        vxFormatImagePatchAddress2d(dst_base[2], x, (y + 1), &dst_addr[2]) };

                    luma1[1][0] = luma1[0][0];
                    cb1[1][0] = cb1[0][0];
                    cr1[1][0] = cr1[0][0];
                }
            }
        }
    }
    status = VX_SUCCESS;
    for (p = 0; p < src_planes; p++)
    {
        status |= vxUnmapImagePatch(src, map_id[p]);
    }
    for (p = 0; p < dst_planes; p++)
    {
        status |= vxUnmapImagePatch(dst, result_map_id[p]);
    }
    if (status != VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to set image patches on source or destination\n");
    }

    return status;
}