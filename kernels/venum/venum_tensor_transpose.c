#include <venum.h>
#include "conversion_utils.h"
#include "tensor_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VX/vx_types.h>
#include <arm_neon.h>
#include <assert.h>

vx_status TransposeTensorKernelImpl(vx_tensor in1, vx_scalar sc_dim1, vx_scalar sc_dim2, vx_tensor out, vx_size el_size)
{
    vx_status status = VX_SUCCESS;

    void* in1_data = NULL;
    void* out_data = NULL;

    vx_size dim1, dim2;

    vx_size in1_dim_num = 0, in1_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, in1_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };
    vx_size out_dim_num = 0, out_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, out_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };

    status |= AllocatePatch(in1, &in1_dim_num, in1_dims, in1_strides, &in1_data, VX_READ_ONLY);
    status |= AllocatePatch(out, &out_dim_num, out_dims, out_strides, &out_data, VX_WRITE_ONLY);

    status |= vxCopyScalar(sc_dim1, &dim1, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(sc_dim2, &dim2, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vx_size out_size = ComputeNumberOfElements(out_dims, out_dim_num);

    if (dim1 == dim2)
    {
        memcpy((vx_uint8 *)out_data, (vx_uint8 *)in1_data, el_size*out_size);
    }
    else
    {
        int tmp = out_strides[dim1];
        out_strides[dim1] = out_strides[dim2];
        out_strides[dim2] = tmp;
        switch (el_size)
        {
        case 1:
        {
            vx_uint8 *dst = (vx_uint8*)out_data;
            vx_uint8 *src = (vx_uint8*)in1_data;
            vx_int32 step_16 = 16;
            vx_uint32 roiw16 = out_size >= step_16 - 1 ? out_size - (step_16 - 1) : 0;
            vx_uint8 tmpsrc[16] = { 0 };
            vx_size i = 0;

            for (; i < roiw16; i += step_16)
            {
                uint8x16_t v_src00 = vld1q_u8(src + i);
                vst1q_u8(tmpsrc, v_src00);

                for (vx_size j = 0; j < 16; j++)
                {
                    vx_size out_pos = ComputeGlobalPositionsFromIndex(i + j, in1_dims, out_strides, in1_dim_num, &out_pos);
                    dst[out_pos] = tmpsrc[j];
                }
            }
            for (; i < out_size; i++)
            {
                vx_size in1_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, in1_strides, in1_dim_num, &in1_pos);
                vx_size out_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, out_strides, in1_dim_num, &out_pos);

                dst[out_pos] = src[in1_pos];
            }
        }
        break;

        case 2:
        {
            vx_int16 *dst = (vx_int16*)out_data;
            vx_int16 *src = (vx_int16*)in1_data;

            int step_xy = 4;

            int width = in1_dims[0], height = in1_dims[1];

            if (in1_dim_num == 2 && height % step_xy == 0)
            {
                for (int y = 0; y < height; y += step_xy)
                {
                    int x = 0;
                    for (; x <= width - step_xy; x += step_xy)
                    {
                        uint16x4_t row0 = vld1_u16(src + (0 + y) * width + x);
                        uint16x4_t row1 = vld1_u16(src + (1 + y) * width + x);
                        uint16x4_t row2 = vld1_u16(src + (2 + y) * width + x);
                        uint16x4_t row3 = vld1_u16(src + (3 + y) * width + x);

                        // Transpose 2x2
                        uint16x4x2_t k0_u16 = vtrn_u16(row0, row1);
                        uint16x4x2_t k1_u16 = vtrn_u16(row2, row3);

                        // Transpose 4x4
                        uint32x2x2_t k0_u32 = vtrn_u32(vreinterpret_u32_u16(k0_u16.val[0]), vreinterpret_u32_u16(k1_u16.val[0]));
                        uint32x2x2_t k1_u32 = vtrn_u32(vreinterpret_u32_u16(k0_u16.val[1]), vreinterpret_u32_u16(k1_u16.val[1]));

                        vst1_u16(dst + (0 + x) * height + y, vreinterpret_u16_u32(k0_u32.val[0]));
                        vst1_u16(dst + (1 + x) * height + y, vreinterpret_u16_u32(k1_u32.val[0]));
                        vst1_u16(dst + (2 + x) * height + y, vreinterpret_u16_u32(k0_u32.val[1]));
                        vst1_u16(dst + (3 + x) * height + y, vreinterpret_u16_u32(k1_u32.val[1]));
                    }
                    for (; x < width; ++x)
                    {
                        vx_uint16 row0 = *(src + (0 + y) * width + x);
                        vx_uint16 row1 = *(src + (1 + y) * width + x);
                        vx_uint16 row2 = *(src + (2 + y) * width + x);
                        vx_uint16 row3 = *(src + (3 + y) * width + x);

                        uint16x4_t result = vdup_n_u16(0);
                        result = vset_lane_u16(row0, result, 0);
                        result = vset_lane_u16(row1, result, 1);
                        result = vset_lane_u16(row2, result, 2);
                        result = vset_lane_u16(row3, result, 3);

                        vst1_u16(dst + x * height + y, result);
                    }
                }
            }
            else
            {
                vx_int32 step_16 = 16;
                vx_uint32 roiw16 = out_size >= step_16 - 1 ? out_size - (step_16 - 1) : 0;
                vx_int16 tmpsrc[16] = { 0 };
                vx_size i = 0;

                for (; i < roiw16; i += step_16)
                {

                    int16x8_t v_src00 = vld1q_s16(src + i);
                    int16x8_t v_src01 = vld1q_s16(src + i + 8);

                    vst1q_s16(tmpsrc, v_src00);
                    vst1q_s16(tmpsrc + 8, v_src01);

                    for (vx_size j = 0; j < 16; j++)
                    {
                        vx_size out_pos = ComputeGlobalPositionsFromIndex(i + j, in1_dims, out_strides, in1_dim_num, &out_pos);
                        dst[out_pos / 2] = tmpsrc[j];
                    }
                }

                for (; i < out_size; i++)
                {

                    vx_size in1_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, in1_strides, in1_dim_num, &in1_pos);
                    vx_size out_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, out_strides, in1_dim_num, &out_pos);

                    dst[out_pos / 2] = src[in1_pos / 2];

                }
            }
            break;

        default:
            for (vx_size i = 0; i < out_size; ++i)
            {
                vx_size in1_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, in1_strides, in1_dim_num, &in1_pos);
                vx_size out_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, out_strides, in1_dim_num, &out_pos);

                memcpy((vx_uint8 *)out_data + out_pos, (vx_uint8 *)in1_data + in1_pos, el_size);
            }
        }
        }
        tmp = out_strides[dim1];
        out_strides[dim1] = out_strides[dim2];
        out_strides[dim2] = tmp;
    }
    status |= ReleasePatch(in1, in1_dim_num, in1_dims, in1_strides, &in1_data, VX_READ_ONLY);
    status |= ReleasePatch(out, out_dim_num, out_dims, out_strides, &out_data, VX_WRITE_ONLY);

    return status;
}
