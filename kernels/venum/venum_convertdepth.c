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

#include <VX/vx_types.h>
#include <VX/vx_lib_extras.h>
#include <arm_neon.h>

static vx_status vxConDeptU8S16(void *src_base,void *dst_base,vx_uint32 height,vx_uint32 width,vx_int32 shift)
{
    vx_int32 x,y;

    vx_uint32 roiw16 = width >= 15 ? height - 15 : 0;
    vx_uint32 roiw8 = width >= 7 ? height - 7 : 0;

    int16x8_t sh=vdupq_n_s16(shift);

    for (y = 0; y < height; y++)
    {
        vx_uint8* srcp = (vx_uint8 *)src_base + y * width;
        vx_int16* dstp = (vx_int16 *)dst_base + y * width;	

        x = 0;
        for (; x < roiw16; x+=16)
        {
            uint8x16_t v_src = vld1q_u8(srcp);
            int16x8_t v_dst0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v_src)));
            int16x8_t v_dst1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(v_src)));

            vst1q_s16(dstp, vshlq_s16(v_dst0, sh));
            vst1q_s16(dstp+8, vshlq_s16(v_dst1, sh));

            srcp+=16;
            dstp+=16;

        }
        for (; x < roiw8; x += 8)
        {
            int16x8_t v_dst = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(srcp)));
            vst1q_s16(dstp, vshlq_s16(v_dst,sh));
            srcp+=8;
            dstp+=8;
        }
        for (; x < width; x++)
        {
            *dstp = ((vx_int16)(*srcp))<<shift;
            srcp++;
            dstp++;
        }
    }
    return VX_SUCCESS;

}
static vx_status vxConDeptS16U8(void *src_base,void *dst_base,vx_uint32 height,vx_uint32 width,vx_int32 shift,vx_enum policy)
{
    vx_int32 x,y;

    vx_uint32 roiw16 = width >= 15 ? height - 15 : 0;
    vx_uint32 roiw8 = width >= 7 ? height - 7 : 0;

    int16x8_t sh=vdupq_n_s16(-shift);

    for (y = 0; y < height; y++)
    {
        vx_int16* srcp = (vx_int16 *)src_base + y * width;
        vx_uint8* dstp = (vx_uint8 *)dst_base + y * width;

        x = 0;
        for (; x < roiw16; x+=16)
        {
            int16x8_t v_src0 = vld1q_s16(srcp);
            int16x8_t v_src1 = vld1q_s16(srcp+8);

            if (policy == VX_CONVERT_POLICY_SATURATE)
            {
                int16x8_t v_dst0= vqshlq_s16(v_src0,sh);
                int16x8_t v_dst1= vqshlq_s16(v_src1,sh);
                uint8x8_t v_dst00 = vqmovun_s16(v_dst0);
                uint8x8_t v_dst01 = vqmovun_s16(v_dst1);
                uint8x16_t v_dst = vcombine_u8(v_dst00,v_dst01);

                vst1q_u8(dstp, v_dst);
            }
            else if (policy == VX_CONVERT_POLICY_WRAP)
            {
                int16x8_t v_dst0= vshlq_s16(v_src0,sh);
                int16x8_t v_dst1= vshlq_s16(v_src1,sh);
                uint8x16_t v_dst = vcombine_u8(vmovn_u16(vreinterpretq_u16_s16(v_dst0)),vmovn_u16(vreinterpretq_u16_s16(v_dst1)));

                vst1q_u8(dstp, v_dst);
            }
            srcp+=16;
            dstp+=16;
        }

        for (; x <width; x++)
        {
            if (policy == VX_CONVERT_POLICY_WRAP)
            {
                *dstp = (vx_uint8)((*srcp) >> shift);
            }
            else if (policy == VX_CONVERT_POLICY_SATURATE)
            {
                vx_int16 value = (*srcp) >> shift;
                value = (value < 0 ? 0 : value);
                value = (value > UINT8_MAX ? UINT8_MAX : value);
                *dstp = (vx_uint8)value;
            }
            srcp++;
            dstp++;
        }
    }
}

// nodeless version of the ConvertDepth kernel
vx_status vxConvertDepth(vx_image input, vx_image output, vx_scalar spol, vx_scalar sshf)
{
    vx_uint32 y, x;
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
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    int16x8_t sh=vdupq_n_s16(0-shift);

    vx_uint32 roiw16 = src_addr.dim_x >= 15 ? src_addr.dim_x - 15 : 0;
    vx_uint32 roiw8 = src_addr.dim_x >= 7 ? src_addr.dim_x - 7 : 0;

    switch (format[0])
    {
        case  VX_DF_IMAGE_U8:
        {
            int16x8_t sh = vdupq_n_s16(shift);

            switch (format[1])
            {
                case VX_DF_IMAGE_S16:
                {
                    vxConDeptU8S16(src_base, dst_base, src_addr.dim_y, src_addr.dim_x, shift);
                }
                break;
            }
        }
        break;

        case VX_DF_IMAGE_S16:
        {
            switch (format[1])
            {
                case VX_DF_IMAGE_U8:
                {
                    vxConDeptS16U8(src_base, dst_base, src_addr.dim_y, src_addr.dim_x, shift, policy);
                }
                break;
            }
        }
        break;
    }

    status |= vxUnmapImagePatch(input, map_id);
    status |= vxUnmapImagePatch(output, result_map_id);

    return status;
}