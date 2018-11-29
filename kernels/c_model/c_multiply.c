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

#include <c_model.h>

// nodeless version of the Multiply kernel
vx_status vxMultiply(vx_image in0, vx_image in1, vx_scalar scale_param, vx_scalar opolicy_param, vx_scalar rpolicy_param, vx_image output)
{
    vx_float32 scale = 0.0f;
    vx_enum overflow_policy = -1;
    vx_enum rounding_policy = -1;
    vx_uint32 y, x;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_df_image in0_format = 0;
    vx_df_image in1_format = 0;
    vx_df_image out_format = 0;
    vx_status status = VX_FAILURE;

    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(in0, VX_IMAGE_FORMAT, &in0_format, sizeof(in0_format));
    vxQueryImage(in1, VX_IMAGE_FORMAT, &in1_format, sizeof(in1_format));

    status = vxGetValidRegionImage(in0, &rect);
    status |= vxAccessImagePatch(in0, &rect, 0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY);
    status |= vxAccessImagePatch(in1, &rect, 0, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
    status |= vxCopyScalar(scale_param, &scale, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(opolicy_param, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(rpolicy_param, &rounding_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    for (y = 0; y < dst_addr.dim_y; y++)
    {
        for (x = 0; x < dst_addr.dim_x; x++)
        {
            /* Either image may be U8 or S16. */
            void *src0p = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
            void *src1p = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
            void *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            /* Convert inputs to int32, perform the primary multiplication. */
            vx_int32 src0 = in0_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)src0p : *(vx_int16 *)src0p;
            vx_int32 src1 = in1_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)src1p : *(vx_int16 *)src1p;
            vx_int32 unscaled_unconverted_result = src0 * src1;

            /*
             * Convert the intermediate result to float64, then scale down
             * that number.
             */
            vx_float64 unscaled_result = (vx_float64)unscaled_unconverted_result;
            vx_float64 scaled_result = scale * unscaled_result;

            /*
             * We need to first convert to an integer type where the
             * possible values fit, or else we get platform-specific
             * overflow behavior for truncation. For saturation, we just
             * avoid further floating-point operations and rely on that a
             * floating-point value overflowing a vx_int32 would similarly
             * overflow a vx_int16 and vx_uint8.
             */
            vx_int32 int_typed_result = (vx_int32)scaled_result;

            vx_int32 final_result_value;

            /* Finally, overflow-check as per the target type and policy. */
            if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
            {
                if (out_format == VX_DF_IMAGE_U8)
                {
                    if (int_typed_result > UINT8_MAX)
                        final_result_value = UINT8_MAX;
                    else if (int_typed_result < 0)
                        final_result_value = 0;
                    else
                        final_result_value = int_typed_result;
                }
                else /* Already verified to be VX_DF_IMAGE_S16 */
                {
                    if (int_typed_result > INT16_MAX)
                        final_result_value = INT16_MAX;
                    else if (int_typed_result < INT16_MIN)
                        final_result_value = INT16_MIN;
                    else
                        final_result_value = int_typed_result;
                }
            }
            else /* Already verified to be VX_CONVERT_POLICY_WRAP. */
            {
                /* Just for show: the final assignment will wrap too. */
                final_result_value = (out_format == VX_DF_IMAGE_U8) ?
                    (vx_uint8)int_typed_result : (vx_int16)int_typed_result;
            }

            if (out_format == VX_DF_IMAGE_U8)
              *(vx_uint8 *)dstp = (vx_uint8)final_result_value;
            else
              *(vx_int16 *)dstp = (vx_int16)final_result_value;
        }
    }
    status |= vxCommitImagePatch(in0, NULL, 0, &src_addr[0], src_base[0]);
    status |= vxCommitImagePatch(in1, NULL, 0, &src_addr[1], src_base[1]);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);

    return status;
}

