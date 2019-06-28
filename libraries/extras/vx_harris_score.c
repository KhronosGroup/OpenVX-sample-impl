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
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>
#include <stdio.h>
#ifdef EXPERIMENTAL_USE_VENUM
#include <arm_neon.h>
#endif

#ifndef OWN_MAX
#define OWN_MAX(a, b) (a) > (b) ? (a) : (b)
#endif

#ifndef OWN_MIN
#define OWN_MIN(a, b) (a) < (b) ? (a) : (b)
#endif

static
vx_param_description_t harris_score_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
};
#if EXPERIMENTAL_USE_VENUM1

static inline float32x4_t harris_score(float32x4_t gx2, float32x4_t gy2, float32x4_t gxgy, float32x4_t sensitivity)
{
    // Trace^2
    float32x4_t trace2 = vaddq_f32(gx2, gy2);
    trace2             = vmulq_f32(trace2, trace2);

    // Det(A)
    float32x4_t det = vmulq_f32(gx2, gy2);
    det             = vmlsq_f32(det, gxgy, gxgy);

    // Det(A) - sensitivity * trace^2
    const float32x4_t mc = vmlsq_f32(det, sensitivity, trace2);

    return mc;

}

static inline void harris_score1x3_FLOAT_FLOAT_FLOAT(float32x4_t low_gx, float32x4_t low_gy, float32x4_t high_gx, float32x4_t high_gy, float32x4_t *gx2, float32x4_t *gy2, float32x4_t *gxgy,
                                              float32x4_t norm_factor)
{
    // Normalize
    low_gx  = vmulq_f32(low_gx, norm_factor);
    high_gx = vmulq_f32(high_gx, norm_factor);

    low_gy  = vmulq_f32(low_gy, norm_factor);
    high_gy = vmulq_f32(high_gy, norm_factor);

    const float32x4_t l_gx = low_gx;
    const float32x4_t l_gy = low_gy;
    
    const float32x4_t m_gx = vextq_f32(low_gx, high_gx, 1);
    const float32x4_t r_gx = vextq_f32(low_gx, high_gx, 2);

    const float32x4_t m_gy = vextq_f32(low_gy, high_gy, 1);
    const float32x4_t r_gy = vextq_f32(low_gy, high_gy, 2);

    // Gx*Gx
    *gx2 = vmlaq_f32(*gx2, l_gx, l_gx);
    *gx2 = vmlaq_f32(*gx2, m_gx, m_gx);
    *gx2 = vmlaq_f32(*gx2, r_gx, r_gx);

    // Gy*Gy
    *gy2 = vmlaq_f32(*gy2, l_gy, l_gy);
    *gy2 = vmlaq_f32(*gy2, m_gy, m_gy);
    *gy2 = vmlaq_f32(*gy2, r_gy, r_gy);

    // Gx*Gy
    *gxgy = vmlaq_f32(*gxgy, l_gx, l_gy);
    *gxgy = vmlaq_f32(*gxgy, m_gx, m_gy);
    *gxgy = vmlaq_f32(*gxgy, r_gx, r_gy);
}
static vx_status harris_score1x3(vx_border_t borders, vx_uint32 grad_size, vx_uint32 height, vx_uint32 width,void* gx_base,void* gy_base,void* dst_base, vx_float32 sensitivity)
{
    vx_status status = VX_SUCCESS;
    if (borders.mode == VX_BORDER_UNDEFINED)
    {
        vx_float32 scale = 1.0 / ((1 << (grad_size - 1)) * 3 * 255.0);
        float32x4_t    v_sensitivity     = vdupq_n_f32(sensitivity);
        float32x4_t    v_norm_factor     = vdupq_n_f32(scale);

        for (vx_int32 y = 2; y < height - 2; y++)
        {
            vx_float32* top_gx = (vx_float32 *)gx_base + (y - 1) * width+1;
            vx_float32* mid_gx = (vx_float32 *)gx_base + (y)* width+1;
            vx_float32* bot_gx = (vx_float32 *)gx_base + (y + 1)* width+1;

            vx_float32* top_gy = (vx_float32 *)gy_base + (y - 1) * width+1;
            vx_float32* mid_gy = (vx_float32 *)gy_base + (y)* width+1;
            vx_float32* bot_gy = (vx_float32 *)gy_base + (y + 1)* width+1;

            vx_float32* pmc = (vx_float32 *)dst_base + y * width+2;

            vx_int32 x = 0;

            for (; x <=width - 4 - 8; x+=8)
            {
                // Gx^2, Gy^2 and Gx*Gy
                float32x4x2_t gx2 =
                {
                    {
                        vdupq_n_f32(0.0f),
                        vdupq_n_f32(0.0f)
                    }
                };
                float32x4x2_t gy2 =
                {
                    {
                        vdupq_n_f32(0.0f),
                        vdupq_n_f32(0.0f)
                    }
                };
                float32x4x2_t gxgy =
                {
                    {
                        vdupq_n_f32(0.0f),
                        vdupq_n_f32(0.0f)
                    }
                };
                // Row0
                float32x4_t low_gx  = vld1q_f32(top_gx+x);
                float32x4_t low_gy  = vld1q_f32(top_gy+x);
                float32x4_t high_gx = vld1q_f32(top_gx+x+4);
                float32x4_t high_gy = vld1q_f32(top_gy+x+4);
                harris_score1x3_FLOAT_FLOAT_FLOAT(low_gx, low_gy, high_gx, high_gy, &gx2.val[0], &gy2.val[0], &gxgy.val[0], v_norm_factor);

                low_gx  = vld1q_f32(top_gx+x+4);
                low_gy  = vld1q_f32(top_gy+x+4);
                high_gx = vld1q_f32(top_gx+x+8);
                high_gy = vld1q_f32(top_gx+x+8);
                harris_score1x3_FLOAT_FLOAT_FLOAT(low_gx, low_gy, high_gx, high_gy, &gx2.val[1], &gy2.val[1], &gxgy.val[1], v_norm_factor);

                // Row1
                low_gx  = vld1q_f32(mid_gx+x);
                low_gy  = vld1q_f32(mid_gy+x);
                high_gx = vld1q_f32(mid_gx+x+4);
                high_gy = vld1q_f32(mid_gy+x+4);
                harris_score1x3_FLOAT_FLOAT_FLOAT(low_gx, low_gy, high_gx, high_gy, &gx2.val[0], &gy2.val[0], &gxgy.val[0], v_norm_factor);

                low_gx  = vld1q_f32(mid_gx+x+4);
                low_gy  = vld1q_f32(mid_gy+x+4);
                high_gx = vld1q_f32(mid_gx+x+8);
                high_gy = vld1q_f32(mid_gy+x+8);
                harris_score1x3_FLOAT_FLOAT_FLOAT(low_gx, low_gy, high_gx, high_gy, &gx2.val[1], &gy2.val[1], &gxgy.val[1], v_norm_factor);

                // Row2
                low_gx  = vld1q_f32(bot_gx+x);
                low_gy  = vld1q_f32(bot_gy+x);
                high_gx = vld1q_f32(bot_gx+x+4);
                high_gy = vld1q_f32(bot_gy+x+4);
                harris_score1x3_FLOAT_FLOAT_FLOAT(low_gx, low_gy, high_gx, high_gy, &gx2.val[0], &gy2.val[0], &gxgy.val[0], v_norm_factor);

                low_gx  = vld1q_f32(bot_gx+x+4);
                low_gy  = vld1q_f32(bot_gy+x+4);
                high_gx = vld1q_f32(bot_gx+x+8);
                high_gy = vld1q_f32(bot_gy+x+8);
                harris_score1x3_FLOAT_FLOAT_FLOAT(low_gx, low_gy, high_gx, high_gy, &gx2.val[1], &gy2.val[1], &gxgy.val[1], v_norm_factor);

                // Calculate harris score
                const float32x4x2_t mc =
                {
                    {
                        harris_score(gx2.val[0], gy2.val[0], gxgy.val[0], v_sensitivity),
                        harris_score(gx2.val[1], gy2.val[1], gxgy.val[1], v_sensitivity)
                    }
                };
                // Store score
                vst1q_f32(pmc+ x + 0, mc.val[0]);
                vst1q_f32(pmc+ x + 4, mc.val[1]);

            }
            for (; x < width - 4; x++)
            {
                vx_float64 sum_ix2 = 0.0;
                vx_float64 sum_iy2 = 0.0;
                vx_float64 sum_ixy = 0.0;
                vx_float64 det_A = 0.0;
                vx_float64 trace_A = 0.0;
                vx_float64 ktrace_A2 = 0.0;
                vx_float64 M_c = 0.0;

                for (vx_int32 i = 0; i < 3; i++)
                {
                    sum_ix2 += top_gx[x + i] * top_gx[x + i] * scale * scale;
                    sum_ix2 += mid_gx[x + i] * mid_gx[x + i] * scale * scale;
                    sum_ix2 += bot_gx[x + i] * bot_gx[x + i] * scale * scale;

                    sum_iy2 += top_gy[x + i] * top_gy[x + i] * scale * scale;
                    sum_iy2 += mid_gy[x + i] * mid_gy[x + i] * scale * scale;
                    sum_iy2 += bot_gy[x + i] * bot_gy[x + i] * scale * scale;

                    sum_ixy += top_gx[x + i] * top_gy[x + i] * scale * scale;
                    sum_ixy += mid_gx[x + i] * mid_gy[x + i] * scale * scale;
                    sum_ixy += bot_gx[x + i] * bot_gy[x + i] * scale * scale;

                }

                det_A = (sum_ix2 * sum_iy2) - (sum_ixy * sum_ixy);
                trace_A = sum_ix2 + sum_iy2;
                ktrace_A2 = (sensitivity * (trace_A * trace_A));
                M_c = det_A - ktrace_A2;
                pmc[x] = (vx_float32)M_c;

            }
        }
    }
    else
    {
        status = VX_ERROR_NOT_IMPLEMENTED;
    }
    return status;
}

static vx_status harris_score1x5(vx_border_t borders, vx_uint32 grad_size, vx_uint32 height, vx_uint32 width, void* gx_base, void* gy_base, void* dst_base, vx_float32 sensitivity)
{
    vx_status status = VX_SUCCESS;
    if (borders.mode == VX_BORDER_UNDEFINED)
    {
        vx_float64 scale = 1.0 / ((1 << (grad_size - 1)) * 5 * 255.0);


        for (vx_int32 y = 3; y < height - 3; y++)
        {
            vx_float32* top2_gx = (vx_float32 *)gx_base + (y - 2) * width+1;
            vx_float32* top1_gx = (vx_float32 *)gx_base + (y - 1) * width+1;
            vx_float32* mid_gx = (vx_float32 *)gx_base + (y)* width+1;
            vx_float32* bot1_gx = (vx_float32 *)gx_base + (y + 1)* width+1;
            vx_float32* bot2_gx = (vx_float32 *)gx_base + (y + 2)* width+1;

            vx_float32* top2_gy = (vx_float32 *)gy_base + (y - 2) * width+1;
            vx_float32* top1_gy = (vx_float32 *)gy_base + (y - 1) * width+1;
            vx_float32* mid_gy = (vx_float32 *)gy_base + (y)* width+1;
            vx_float32* bot1_gy = (vx_float32 *)gy_base + (y + 1)* width+1;
            vx_float32* bot2_gy = (vx_float32 *)gy_base + (y + 2)* width+1;

            vx_float32* pmc = (vx_float32 *)dst_base + y * width+3;

            for (vx_int32 x = 0; x < width - 6; x++)
            {
                vx_float64 sum_ix2 = 0.0;
                vx_float64 sum_iy2 = 0.0;
                vx_float64 sum_ixy = 0.0;
                vx_float64 det_A = 0.0;
                vx_float64 trace_A = 0.0;
                vx_float64 ktrace_A2 = 0.0;
                vx_float64 M_c = 0.0;
                for (vx_int32 i = 0; i < 5; i++)
                {
                    sum_ix2 += top2_gx[x+i] * top2_gx[x+i] * scale * scale             ;
                    sum_ix2 += top1_gx[x+i] * top1_gx[x+i] * scale * scale;
                    sum_ix2 += mid_gx[x+i] * mid_gx[x+i] * scale * scale;
                    sum_ix2 += bot1_gx[x+i] * bot1_gx[x+i] * scale * scale;
                    sum_ix2 += bot2_gx[x+i] * bot2_gx[x+i] * scale * scale;

                    sum_iy2 += top2_gy[x + i] * top2_gy[x + i] * scale * scale;
                    sum_iy2 += top1_gy[x + i] * top1_gy[x + i] * scale * scale;
                    sum_iy2 += mid_gy[x + i] * mid_gy[x + i] * scale * scale;
                    sum_iy2 += bot1_gy[x + i] * bot1_gy[x + i] * scale * scale;
                    sum_iy2 += bot2_gy[x + i] * bot2_gy[x + i] * scale * scale;

                    sum_ixy += top2_gx[x + i] * top2_gy[x + i] * scale * scale;
                    sum_ixy += top1_gx[x + i] * top1_gy[x + i] * scale * scale;
                    sum_ixy += mid_gx[x + i] * mid_gy[x + i] * scale * scale;
                    sum_ixy += bot1_gx[x + i] * bot1_gy[x + i] * scale * scale;
                    sum_ixy += bot2_gx[x + i] * bot2_gy[x + i] * scale * scale;

                }

                det_A = (sum_ix2 * sum_iy2) - (sum_ixy * sum_ixy);
                trace_A = sum_ix2 + sum_iy2;
                ktrace_A2 = (sensitivity * (trace_A * trace_A));

                M_c = det_A - ktrace_A2;

                pmc[x] = (vx_float32)M_c;

            }
        }
    }
    else
    {
        status = VX_ERROR_NOT_IMPLEMENTED;
    }
    return status;
}
static vx_status harris_score1x7(vx_border_t borders, vx_uint32 grad_size, vx_uint32 height, vx_uint32 width, void* gx_base, void* gy_base, void* dst_base, vx_float32 sensitivity)
{
    vx_status status = VX_SUCCESS;
    if (borders.mode == VX_BORDER_UNDEFINED)
    {
        vx_float64 scale = 1.0 / ((1 << (grad_size - 1)) * 7 * 255.0);


        for (vx_int32 y = 4; y < height - 4; y++)
        {
            vx_float32* top3_gx = (vx_float32 *)gx_base + (y - 3) * width+1;
            vx_float32* top2_gx = (vx_float32 *)gx_base + (y - 2) * width+1;
            vx_float32* top1_gx = (vx_float32 *)gx_base + (y - 1) * width+1;
            vx_float32* mid_gx = (vx_float32 *)gx_base + (y)* width+1;
            vx_float32* bot1_gx = (vx_float32 *)gx_base + (y + 1)* width+1;
            vx_float32* bot2_gx = (vx_float32 *)gx_base + (y + 2)* width+1;
            vx_float32* bot3_gx = (vx_float32 *)gx_base + (y + 3)* width+1;

            vx_float32* top3_gy = (vx_float32 *)gy_base + (y - 3) * width+1;
            vx_float32* top2_gy = (vx_float32 *)gy_base + (y - 2) * width+1;
            vx_float32* top1_gy = (vx_float32 *)gy_base + (y - 1) * width+1;
            vx_float32* mid_gy = (vx_float32 *)gy_base + (y)* width+1;
            vx_float32* bot1_gy = (vx_float32 *)gy_base + (y + 1)* width+1;
            vx_float32* bot2_gy = (vx_float32 *)gy_base + (y + 2)* width+1;
            vx_float32* bot3_gy = (vx_float32 *)gy_base + (y + 3)* width+1;

            vx_float32* pmc = (vx_float32 *)dst_base + y * width + 4;

            for (vx_int32 x = 0; x < width - 8; x++)
            {
                vx_float64 sum_ix2 = 0.0;
                vx_float64 sum_iy2 = 0.0;
                vx_float64 sum_ixy = 0.0;
                vx_float64 det_A = 0.0;
                vx_float64 trace_A = 0.0;
                vx_float64 ktrace_A2 = 0.0;
                vx_float64 M_c = 0.0;
                for (vx_int32 i = 0; i < 7; i++)
                {
                    sum_ix2 += top3_gx[x + i] * top3_gx[x + i] * scale * scale;
                    sum_ix2 += top2_gx[x + i] * top2_gx[x + i] * scale * scale;
                    sum_ix2 += top1_gx[x + i] * top1_gx[x + i] * scale * scale;
                    sum_ix2 += mid_gx[x + i] * mid_gx[x + i] * scale * scale;
                    sum_ix2 += bot1_gx[x + i] * bot1_gx[x + i] * scale * scale;
                    sum_ix2 += bot2_gx[x + i] * bot2_gx[x + i] * scale * scale;
                    sum_ix2 += bot3_gx[x + i] * bot3_gx[x + i] * scale * scale;

                    sum_iy2 += top3_gy[x + i] * top3_gy[x + i] * scale * scale;
                    sum_iy2 += top2_gy[x + i] * top2_gy[x + i] * scale * scale;
                    sum_iy2 += top1_gy[x + i] * top1_gy[x + i] * scale * scale;
                    sum_iy2 += mid_gy[x + i] * mid_gy[x + i] * scale * scale;
                    sum_iy2 += bot1_gy[x + i] * bot1_gy[x + i] * scale * scale;
                    sum_iy2 += bot2_gy[x + i] * bot2_gy[x + i] * scale * scale;
                    sum_iy2 += bot3_gy[x + i] * bot3_gy[x + i] * scale * scale;

                    sum_ixy += top3_gx[x + i] * top3_gy[x + i] * scale * scale;
                    sum_ixy += top2_gx[x + i] * top2_gy[x + i] * scale * scale;
                    sum_ixy += top1_gx[x + i] * top1_gy[x + i] * scale * scale;
                    sum_ixy += mid_gx[x + i] * mid_gy[x + i] * scale * scale;
                    sum_ixy += bot1_gx[x + i] * bot1_gy[x + i] * scale * scale;
                    sum_ixy += bot2_gx[x + i] * bot2_gy[x + i] * scale * scale;
                    sum_ixy += bot3_gx[x + i] * bot3_gy[x + i] * scale * scale;
                }

                det_A = (sum_ix2 * sum_iy2) - (sum_ixy * sum_ixy);
                trace_A = sum_ix2 + sum_iy2;
                ktrace_A2 = (sensitivity * (trace_A * trace_A));

                M_c = det_A - ktrace_A2;

                pmc[x] = (vx_float32)M_c;

            }
        }
    }
    else
    {
        status = VX_ERROR_NOT_IMPLEMENTED;
    }
    return status;
}
static
vx_status VX_CALLBACK ownHarrisScoreKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(harris_score_kernel_params))
    {
        vx_image  grad_x = (vx_image)parameters[0];
        vx_image  grad_y = (vx_image)parameters[1];
        vx_scalar sensitivity = (vx_scalar)parameters[2];
        vx_scalar grad_s = (vx_scalar)parameters[3];
        vx_scalar winds = (vx_scalar)parameters[4];
        vx_image  dst = (vx_image)parameters[5];

        vx_float32 k = 0.0f;
        vx_uint32 block_size = 0;
        vx_uint32 grad_size = 0;
        vx_rectangle_t rect;

        status = vxGetValidRegionImage(grad_x, &rect);

        status |= vxCopyScalar(grad_s, &grad_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar(winds, &block_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar(sensitivity, &k, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

        if (status == VX_SUCCESS)
        {
            vx_int32 x;
            vx_int32 y;
            vx_int32 i;
            vx_int32 j;
            void* gx_base = NULL;
            void* gy_base = NULL;
            void* dst_base = NULL;
            vx_imagepatch_addressing_t gx_addr = VX_IMAGEPATCH_ADDR_INIT;
            vx_imagepatch_addressing_t gy_addr = VX_IMAGEPATCH_ADDR_INIT;
            vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
            vx_map_id grad_x_map_id = 0;
            vx_map_id grad_y_map_id = 0;
            vx_map_id dst_map_id = 0;
            vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

            status |= vxMapImagePatch(grad_x, &rect, 0, &grad_x_map_id, &gx_addr, &gx_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
            status |= vxMapImagePatch(grad_y, &rect, 0, &grad_y_map_id, &gy_addr, &gy_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
            status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

            status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

            if (VX_SUCCESS == status)
            {
                
                if (block_size == 3)
                {
                    status |= harris_score1x3(borders, grad_size, gx_addr.dim_y, gx_addr.dim_x, gx_base, gy_base, dst_base,k);
                
                }
                else if (block_size == 5)
                {
                    status |= harris_score1x5(borders, grad_size, gx_addr.dim_y, gx_addr.dim_x, gx_base, gy_base, dst_base, k);
                }
                else if (block_size == 7)
                {
                    status |= harris_score1x7(borders, grad_size, gx_addr.dim_y, gx_addr.dim_x, gx_base, gy_base, dst_base, k);
                }
                else
                {
                    /*! \todo implement other Harris Corners border modes */
                    if (borders.mode == VX_BORDER_UNDEFINED)
                    {
                        vx_float64 scale = 1.0 / ((1 << (grad_size - 1)) * block_size * 255.0);

                        vx_int32 b = (block_size / 2) + 1;
                        vx_int32 b2 = (block_size / 2);

                        vxAlterRectangle(&rect, b, b, -b, -b);

                        for (y = b; (y < (vx_int32)(gx_addr.dim_y - b)); y++)
                        {
                            for (x = b; x < (vx_int32)(gx_addr.dim_x - b); x++)
                            {
                                vx_float64 sum_ix2 = 0.0;
                                vx_float64 sum_iy2 = 0.0;
                                vx_float64 sum_ixy = 0.0;
                                vx_float64 det_A = 0.0;
                                vx_float64 trace_A = 0.0;
                                vx_float64 ktrace_A2 = 0.0;
                                vx_float64 M_c = 0.0;

                                vx_float32* pmc = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                                for (j = -b2; j <= b2; j++)
                                {
                                    for (i = -b2; i <= b2; i++)
                                    {
                                        vx_float32* pgx = vxFormatImagePatchAddress2d(gx_base, x + i, y + j, &gx_addr);
                                        vx_float32* pgy = vxFormatImagePatchAddress2d(gy_base, x + i, y + j, &gy_addr);

                                        vx_float32 gx = (*pgx);
                                        vx_float32 gy = (*pgy);

                                        sum_ix2 += gx * gx * scale * scale;



                                        sum_iy2 += gy * gy * scale * scale;
                                        sum_ixy += gx * gy * scale * scale;
                                    }
                                }

                                det_A = (sum_ix2 * sum_iy2) - (sum_ixy * sum_ixy);
                                trace_A = sum_ix2 + sum_iy2;
                                ktrace_A2 = (k * (trace_A * trace_A));

                                M_c = det_A - ktrace_A2;

                                *pmc = (vx_float32)M_c;

                            }
                        }
                    }
                    else
                    {
                        status = VX_ERROR_NOT_IMPLEMENTED;
                    }
                }
            }

            status |= vxUnmapImagePatch(grad_x, grad_x_map_id);
            status |= vxUnmapImagePatch(grad_y, grad_y_map_id);
            status |= vxUnmapImagePatch(dst, dst_map_id);
        }
    } // if ptrs non NULL

    return status;
} /* ownHarrisScoreKernel() */
#else
static
vx_status VX_CALLBACK ownHarrisScoreKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(harris_score_kernel_params))
    {
        vx_image  grad_x      = (vx_image)parameters[0];
        vx_image  grad_y      = (vx_image)parameters[1];
        vx_scalar sensitivity = (vx_scalar)parameters[2];
        vx_scalar grad_s      = (vx_scalar)parameters[3];
        vx_scalar winds       = (vx_scalar)parameters[4];
        vx_image  dst         = (vx_image)parameters[5];

        vx_float32 k = 0.0f;
        vx_uint32 block_size = 0;
        vx_uint32 grad_size = 0;
        vx_rectangle_t rect;

        status = vxGetValidRegionImage(grad_x, &rect);

        status |= vxCopyScalar(grad_s, &grad_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar(winds, &block_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar(sensitivity, &k, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

        if (status == VX_SUCCESS)
        {
            vx_int32 x;
            vx_int32 y;
            vx_int32 i;
            vx_int32 j;
            void* gx_base = NULL;
            void* gy_base = NULL;
            void* dst_base = NULL;
            vx_imagepatch_addressing_t gx_addr  = VX_IMAGEPATCH_ADDR_INIT;
            vx_imagepatch_addressing_t gy_addr  = VX_IMAGEPATCH_ADDR_INIT;
            vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
            vx_map_id grad_x_map_id = 0;
            vx_map_id grad_y_map_id = 0;
            vx_map_id dst_map_id = 0;
            vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

            status |= vxMapImagePatch(grad_x, &rect, 0, &grad_x_map_id, &gx_addr, &gx_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
            status |= vxMapImagePatch(grad_y, &rect, 0, &grad_y_map_id, &gy_addr, &gy_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
            status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

            status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

            if (VX_SUCCESS == status)
            {
                /*! \todo implement other Harris Corners border modes */
                if (borders.mode == VX_BORDER_UNDEFINED)
                {
                    vx_float64 scale = 1.0 / ((1 << (grad_size - 1)) * block_size * 255.0);

                    vx_int32 b  = (block_size / 2) + 1;
                    vx_int32 b2 = (block_size / 2);

                    vxAlterRectangle(&rect, b, b, -b, -b);

                    for (y = b; (y < (vx_int32)(gx_addr.dim_y - b)); y++)
                    {
                        for (x = b; x < (vx_int32)(gx_addr.dim_x - b); x++)
                        {
                            vx_float64 sum_ix2   = 0.0;
                            vx_float64 sum_iy2   = 0.0;
                            vx_float64 sum_ixy   = 0.0;
                            vx_float64 det_A     = 0.0;
                            vx_float64 trace_A   = 0.0;
                            vx_float64 ktrace_A2 = 0.0;
                            vx_float64 M_c       = 0.0;

                            vx_float32* pmc = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                            for (j = -b2; j <= b2; j++)
                            {
                                for (i = -b2; i <= b2; i++)
                                {
                                    vx_float32* pgx = vxFormatImagePatchAddress2d(gx_base, x + i, y + j, &gx_addr);
                                    vx_float32* pgy = vxFormatImagePatchAddress2d(gy_base, x + i, y + j, &gy_addr);

                                    vx_float32 gx = (*pgx);
                                    vx_float32 gy = (*pgy);

                                    sum_ix2 += gx * gx * scale * scale;
                                    sum_iy2 += gy * gy * scale * scale;
                                    sum_ixy += gx * gy * scale * scale;
                                }
                            }

                            det_A = (sum_ix2 * sum_iy2) - (sum_ixy * sum_ixy);
                            trace_A = sum_ix2 + sum_iy2;
                            ktrace_A2 = (k * (trace_A * trace_A));

                            M_c = det_A - ktrace_A2;

                            *pmc = (vx_float32)M_c;
#if 0
                            if (sum_ix2 > 0 || sum_iy2 > 0 || sum_ixy > 0)
                            {
                                printf("Σx²=%d Σy²=%d Σxy=%d\n",sum_ix2,sum_iy2,sum_ixy);
                                printf("det|A|=%lld trace(A)=%lld M_c=%lld\n",det_A,trace_A,M_c);
                                printf("{x,y,M_c32}={%d,%d,%d}\n",x,y,M_c32);
                            }
#endif
                        }
                    }
                }
                else
                {
                    status = VX_ERROR_NOT_IMPLEMENTED;
                }
            }

            status |= vxUnmapImagePatch(grad_x, grad_x_map_id);
            status |= vxUnmapImagePatch(grad_y, grad_y_map_id);
            status |= vxUnmapImagePatch(dst, dst_map_id);
        }
    } // if ptrs non NULL

    return status;
} /* ownHarrisScoreKernel() */
#endif
static
vx_status VX_CALLBACK set_harris_score_valid_rectangle(
    vx_node node,
    vx_uint32 index,
    const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && index < dimof(harris_score_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
        vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

        status = vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
        if (VX_SUCCESS != status)
            return status;

        if (VX_BORDER_UNDEFINED == borders.mode)
        {
            vx_parameter param      = 0;
            vx_scalar    block_size = 0;

            param = vxGetParameterByIndex(node, 4);

            if (VX_SUCCESS == vxGetStatus((vx_reference)param) &&
                VX_SUCCESS == vxQueryParameter(param, VX_PARAMETER_REF, &block_size, sizeof(block_size)))
            {
                vx_enum type = 0;

                status = vxQueryScalar(block_size, VX_SCALAR_TYPE, &type, sizeof(type));

                if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
                {
                    vx_int32 win_size = 0;

                    status = vxCopyScalar(block_size, &win_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                    if (VX_SUCCESS == status &&
                        (win_size == 3 || win_size == 5 || win_size == 7))
                    {
                        vx_uint32 border_size = (win_size / 2) + 1;

                        if (input_valid[0]->start_x > input_valid[1]->end_x ||
                            input_valid[0]->end_x   < input_valid[1]->start_x ||
                            input_valid[0]->start_y > input_valid[1]->end_y ||
                            input_valid[0]->end_y   < input_valid[1]->start_y)
                        {
                            /* no intersection */
                            status = VX_ERROR_INVALID_PARAMETERS;
                        }
                        else
                        {
                            output_valid[0]->start_x = OWN_MAX(input_valid[0]->start_x, input_valid[1]->start_x) + border_size;
                            output_valid[0]->start_y = OWN_MAX(input_valid[0]->start_y, input_valid[1]->start_y) + border_size;
                            output_valid[0]->end_x   = OWN_MIN(input_valid[0]->end_x, input_valid[1]->end_x) - border_size;
                            output_valid[0]->end_y   = OWN_MIN(input_valid[0]->end_y, input_valid[1]->end_y) - border_size;
                            status = VX_SUCCESS;
                        }
                    }
                    else
                        status = VX_ERROR_INVALID_PARAMETERS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }
            else
                status = VX_ERROR_INVALID_PARAMETERS;

            if (NULL != block_size)
                vxReleaseScalar(&block_size);

            if (NULL != param)
                vxReleaseParameter(&param);
        } // if BORDER_UNDEFINED
        else
            status = VX_ERROR_NOT_IMPLEMENTED;
    } // if ptrs non NULL

    return status;
} /* set_harris_score_valid_rectangle() */

static
vx_status VX_CALLBACK own_harris_score_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node &&
        num == dimof(harris_score_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;
        vx_parameter param3 = 0;
        vx_parameter param4 = 0;
        vx_parameter param5 = 0;

        vx_image  dx = 0;
        vx_image  dy = 0;
        vx_scalar sensitivity = 0;
        vx_scalar gradient_size = 0;
        vx_scalar block_size = 0;

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);
        param3 = vxGetParameterByIndex(node, 2);
        param4 = vxGetParameterByIndex(node, 3);
        param5 = vxGetParameterByIndex(node, 4);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param3) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param4) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param5))
        {
            status = VX_SUCCESS;

            status |= vxQueryParameter(param1, VX_PARAMETER_REF, &dx, sizeof(dx));
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &dy, sizeof(dy));
            status |= vxQueryParameter(param3, VX_PARAMETER_REF, &sensitivity, sizeof(sensitivity));
            status |= vxQueryParameter(param4, VX_PARAMETER_REF, &gradient_size, sizeof(gradient_size));
            status |= vxQueryParameter(param5, VX_PARAMETER_REF, &block_size, sizeof(block_size));

            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)dx) &&
                VX_SUCCESS == vxGetStatus((vx_reference)dy) &&
                VX_SUCCESS == vxGetStatus((vx_reference)sensitivity) &&
                VX_SUCCESS == vxGetStatus((vx_reference)gradient_size) &&
                VX_SUCCESS == vxGetStatus((vx_reference)block_size))
            {
                vx_uint32   dx_width = 0;
                vx_uint32   dx_height = 0;
                vx_df_image dx_format = 0;

                vx_uint32   dy_width = 0;
                vx_uint32   dy_height = 0;
                vx_df_image dy_format = 0;

                status |= vxQueryImage(dx, VX_IMAGE_WIDTH, &dx_width, sizeof(dx_width));
                status |= vxQueryImage(dx, VX_IMAGE_HEIGHT, &dx_height, sizeof(dx_height));
                status |= vxQueryImage(dx, VX_IMAGE_FORMAT, &dx_format, sizeof(dx_format));

                status |= vxQueryImage(dy, VX_IMAGE_WIDTH, &dy_width, sizeof(dy_width));
                status |= vxQueryImage(dy, VX_IMAGE_HEIGHT, &dy_height, sizeof(dy_height));
                status |= vxQueryImage(dy, VX_IMAGE_FORMAT, &dy_format, sizeof(dy_format));

                /* validate input images */
                if (VX_SUCCESS == status &&
                    dx_format == VX_DF_IMAGE_F32 && dy_format == VX_DF_IMAGE_F32 &&
                    dx_width == dy_width &&
                    dx_height == dy_height)
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_FORMAT;

                /* validate input sensitivity */
                if (VX_SUCCESS == status)
                {
                    vx_enum type = 0;

                    status |= vxQueryScalar(sensitivity, VX_SCALAR_TYPE, &type, sizeof(type));

                    if (VX_SUCCESS == status && VX_TYPE_FLOAT32 == type)
                        status = VX_SUCCESS;
                    else
                        status = VX_ERROR_INVALID_TYPE;
                }

                /* validate input gradient_size */
                if (VX_SUCCESS == status)
                {
                    vx_enum type = 0;

                    status |= vxQueryScalar(gradient_size, VX_SCALAR_TYPE, &type, sizeof(type));

                    if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
                    {
                        vx_int32 size = 0;

                        status |= vxCopyScalar(gradient_size, &size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                        if (VX_SUCCESS == status &&
                            (size == 3 || size == 5 || size == 7))
                        {
                            status = VX_SUCCESS;
                        }
                        else
                            status = VX_ERROR_INVALID_PARAMETERS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_TYPE;
                    }
                }

                /* validate input block_size */
                if (VX_SUCCESS == status)
                {
                    vx_enum type = 0;

                    status |= vxQueryScalar(block_size, VX_SCALAR_TYPE, &type, sizeof(type));

                    if (VX_SUCCESS == status && VX_TYPE_INT32 == type)
                    {
                        vx_int32 size = 0;

                        status |= vxCopyScalar(block_size, &size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                        if (VX_SUCCESS == status &&
                            (size == 3 || size == 5 || size == 7))
                        {
                            status = VX_SUCCESS;
                        }
                        else
                            status = VX_ERROR_INVALID_PARAMETERS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_TYPE;
                    }
                }

                /* validate output image */
                if (VX_SUCCESS == status)
                {
                    vx_df_image dst_format = VX_DF_IMAGE_F32;
                    vx_kernel_image_valid_rectangle_f callback = &set_harris_score_valid_rectangle;

                    status |= vxSetMetaFormatAttribute(metas[5], VX_IMAGE_WIDTH,  &dx_width,   sizeof(dx_width));
                    status |= vxSetMetaFormatAttribute(metas[5], VX_IMAGE_HEIGHT, &dx_height,  sizeof(dx_height));
                    status |= vxSetMetaFormatAttribute(metas[5], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

                    status |= vxSetMetaFormatAttribute(metas[5], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
                }
            }
        }

        if (NULL != dx)
            vxReleaseImage(&dx);

        if (NULL != dy)
            vxReleaseImage(&dy);

        if (NULL != sensitivity)
            vxReleaseScalar(&sensitivity);

        if (NULL != gradient_size)
            vxReleaseScalar(&gradient_size);

        if (NULL != block_size)
            vxReleaseScalar(&block_size);

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);

        if (NULL != param3)
            vxReleaseParameter(&param3);

        if (NULL != param4)
            vxReleaseParameter(&param4);

        if (NULL != param5)
            vxReleaseParameter(&param5);
    } // if ptrs non NULL

    return status;
} /* own_harris_score_validator() */

vx_kernel_description_t harris_score_kernel =
{
    VX_KERNEL_EXTRAS_HARRIS_SCORE,
    "org.khronos.extras.harris_score",
    ownHarrisScoreKernel,
    harris_score_kernel_params, dimof(harris_score_kernel_params),
    own_harris_score_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
