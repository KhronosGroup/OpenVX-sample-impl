/*
 * Copyright (c) 2016-2017 The Khronos Group Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <VX/vx.h>
#include <half/sip_ml_fp16.hpp>

float short2float(int16_t val)
{
    int16_t sign = (val & 0x8000) >> 15;

    if (sign == 1)
    {
        val ^= 0xffff;
        val++;
    }
    int16_t mantisa = (val & 0x7F00) >> 8;
    int16_t fraction = (val & 0x00FF);

    int16_t checker = 0x0001;
    float counter = 0;
    for (int i = 8; i > 0; i--)
    {
        if ((fraction & checker) != 0)
        {
            counter += 1.0f / (1 << i);
        }
        checker <<= 1;
    }

    counter += mantisa;

    if (sign == 1)
    {
        counter = -counter;
    }

    return counter;
}

int32_t conversion_16_8(int32_t prod)
{
    uint32_t msb_s;

    uint32_t sign_bit;

    uint16_t out_tmp_value;
    uint16_t out_frac_value;
    uint16_t out_sign_value;
    int16_t out_put_value;

    sign_bit = prod & 0x80000000;
    msb_s = prod & 0x7F800000;

    if ((sign_bit == 0x80000000) && (msb_s == 0x7F800000))
    {
        out_tmp_value = ((prod & 0x007F0000) >> 8);
        out_frac_value = ((prod & 0x0000FF00) >> 8);
    }

    else if ((sign_bit == 0x80000000) && (msb_s != 0x7F800000))
    {
        out_tmp_value = 0x0000;
        out_frac_value = 0x0000;
    }
    else if ((sign_bit == 0x00000000) && (msb_s != 0x00000000))
    {
        out_tmp_value = 0x7F00;
        out_frac_value = 0x00FF;
    }
    else
    {
        out_tmp_value = ((prod & 0x007F0000) >> 8);
        out_frac_value = ((prod & 0x0000FF00) >> 8);
    }
    out_sign_value = ((prod & 0x80000000) >> 16);
    out_put_value = ((out_tmp_value) | (out_frac_value) | out_sign_value);

    uint32_t test = prod & 0x00000080;
    if (test)
    {
        if (((sign_bit == 0x80000000) && (msb_s == 0x7F800000)) | ((sign_bit == 0x00000000) && (msb_s == 0x00000000)))
        {
            out_put_value = out_put_value + 1;
        }
    }

    return(out_put_value);
}

int32_t conversion_24_8_i(int32_t acc_value)
{
    int32_t out;
    if (acc_value >= 32767)
    {
        out = 0x00007FFF;
    }
    else if (acc_value > -32768)
    {
        out = (acc_value);
    }
    else
    {
        out = 0xFFFF8000;
    }
    return(out);
}

int16_t conversion_24_8(int32_t acc_value)
{
    int16_t out;
    if (acc_value >= 32767)
    {
        out = 0x7FFF;
    }
    else if (acc_value > -32768)
    {
        out = (acc_value & 0x0000FFFF);
    }
    else
    {
        out = 0x8000;
    }
    return(out);
}

// Convert from regular decimal into Q1.7.8 fixed point decimal
int16_t float2short(double x) {
    double x_scaled = x * 256;

    int16_t maxVal = (1 << (16 - 1)) - 1;

    if (x_scaled > maxVal) {
        return maxVal;
    }

    if (x_scaled < -maxVal) {
        return -maxVal;
    }

    return (int16_t) (x_scaled + 0.5);
}


// Convert from regular decimal into Q1.7.8 fixed point decimal
vx_int16 QUANTIZE(double x)
{
    double x_scaled = x * 256;

    vx_int16 maxVal = (1 << (16 - 1)) - 1; // if numBits == 8, the number should be 127, allowing range of [-127,+127]

    if (x_scaled > maxVal)
    {
        return maxVal;
    }

    if (x_scaled < -maxVal)
    {
        return -maxVal;
    }

    float rounded_up = ceilf((float)x_scaled);
    float rounded_down = floorf((float)x_scaled);

    if (fabs(rounded_up - x_scaled) < fabs(x_scaled - rounded_down))
    {
        return (vx_int16)rounded_up;
    }
    else
    {
        return (vx_int16)rounded_down;
    }
}

double UNQUANTIZE(vx_int16 val)
{
    return ((vx_float32)val)/256;
    /*
    vx_int16 sign = (val & 0x8000) >> 15;

    if (sign == 1)
    {
        val ^= 0xffff;
        val++;
    }
    vx_int16 mantisa = (val & 0x7F00) >> 8;
    vx_int16 fraction = (val & 0x00FF);

    vx_int16 checker = 0x0001;
    double counter = 0;
    for (int i = 8; i > 0; i--)
    {
        if ((fraction & checker) != 0)
        {
            counter += 1.0 / (1 << i);
        }
        checker <<= 1;
    }

    counter += mantisa;

    if (sign == 1)
    {
        counter = -counter;
    }

    return counter;
    */
}
