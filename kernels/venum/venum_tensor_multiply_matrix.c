#include <venum.h>

#include <assert.h>

#include <arm_neon.h>

#define Q78_FIXED_POINT_POSITION 8
#define Q78_SCALE   (1 << Q78_FIXED_POINT_POSITION)
#define Q78_HALF    (1 << (Q78_FIXED_POINT_POSITION - 1))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define CLAMP(val, lower, upper) MAX((lower), MIN((val), (upper)))

static void TensorMultiplyMatrix_U8(const vx_uint8 *src1, const vx_uint32 *dim1, const vx_uint32 *srcStride1, const vx_uint8 *src2, const vx_uint32 *dim2, const vx_uint32 *srcStride2, const vx_uint8 *src3, const vx_uint32 *srcStride3, const vx_uint32 *dstStride, vx_uint8 *dst) 
{
    vx_uint32 row, col, index;
    vx_uint32 dim10 = dim1[0];
    vx_uint32 dim11 = dim1[1];
    vx_uint32 dim20 = dim2[0];
    vx_uint32 dim20_neon = dim20 / 4;
    vx_uint32 srcStride10 = srcStride1[0];
    vx_uint32 srcStride11 = srcStride1[1];
    vx_uint32 srcStride20 = srcStride2[0];
    vx_uint32 srcStride21 = srcStride2[1];
    vx_uint32 srcStride30 = srcStride3[0];
    vx_uint32 srcStride31 = srcStride3[1];
    vx_uint32 dstStride0 = dstStride[0];
    vx_uint32 dstStride1 = dstStride[1];

    vx_uint32 sum1, s1, s2[4], s3[4], sum[4];
    uint32x4_t s1u32x4, s2u32x4, s3u32x4, sumu32x4;

    for (row = 0; row < dim11; row++)
    {
        for (col = 0; col < dim20_neon; col += 4)
        {
            sum[0] = 0;
            sum[1] = 0;
            sum[2] = 0;
            sum[3] = 0;

            sumu32x4 = vld1q_u32(sum);
            for (index = 0; index < dim10; index++)
            {
                s1 = src1[srcStride11 * row + srcStride10 * index];
                s2[0] = src2[srcStride21 * index + srcStride20 * (col + 0)];
                s2[1] = src2[srcStride21 * index + srcStride20 * (col + 1)];
                s2[2] = src2[srcStride21 * index + srcStride20 * (col + 2)];
                s2[3] = src2[srcStride21 * index + srcStride20 * (col + 3)];

                s1u32x4 = vdupq_n_u32(s1);
                s2u32x4 = vld1q_u32(s2);

                sumu32x4 = vmlaq_u32(sumu32x4, s1u32x4, s2u32x4);
            }
            if (NULL != src3)
            {
                s3[0] = src3[srcStride31 * row + srcStride30 * (col + 0)];
                s3[1] = src3[srcStride31 * row + srcStride30 * (col + 1)];
                s3[2] = src3[srcStride31 * row + srcStride30 * (col + 2)];
                s3[3] = src3[srcStride31 * row + srcStride30 * (col + 3)];

                s3u32x4 = vld1q_u32(s3);
                sumu32x4 = vaddq_u32(sumu32x4, s3u32x4);
            }

            vst1q_u32(sum, sumu32x4);

            dst[dstStride1 * row + dstStride0 * (col + 0)] = CLAMP(sum[0], 0, UINT8_MAX);
            dst[dstStride1 * row + dstStride0 * (col + 1)] = CLAMP(sum[1], 0, UINT8_MAX);
            dst[dstStride1 * row + dstStride0 * (col + 2)] = CLAMP(sum[2], 0, UINT8_MAX);
            dst[dstStride1 * row + dstStride0 * (col + 3)] = CLAMP(sum[3], 0, UINT8_MAX);
        }

        for (col = dim20_neon; col < dim20; col++) 
        {
            sum1 = 0;
            for (index = 0; index < dim10; index++) 
            {
                sum1 += src1[srcStride11 * row + srcStride10 * index] * src2[srcStride21 * index + srcStride20 * col];
            }
            if (NULL != src3) 
            {
                sum1 += src3[srcStride31 * row + srcStride30 * col];
            }
            dst[dstStride1 * row + dstStride0 * col] = CLAMP(sum1, 0, UINT8_MAX);
        }
    }
}

static void TensorMultiplyMatrix_S8(const vx_int8 *src1, const vx_uint32 *dim1, const vx_uint32 *srcStride1, const vx_int8 *src2, const vx_uint32 *dim2, const vx_uint32 *srcStride2, const vx_int8 *src3, const vx_uint32 *srcStride3, const vx_uint32 *dstStride, vx_int8 *dst) 
{
    vx_uint32 row, col, index;
    vx_uint32 dim1_multiple4 = dim1[0] - dim1[0] % 4;
    vx_uint32 dim2_multiple4 = dim2[0] - dim2[0] % 4;

    vx_int32 sum, d[4], s1[4], s2[4], s3[4], t[4];

    for (row = 0; row < dim1[1]; row++) 
    {
        for (col = 0; col < dim2_multiple4; col+=4) 
        {   
            for(vx_uint32 k = 0; k < 4; k++)
            {
                d[k] = 0;
                for (index = 0; index < dim1_multiple4; index+=4) 
                {
                    s1[0] = src1[srcStride1[1] * row + srcStride1[0] * (index + 0)];
                    s1[1] = src1[srcStride1[1] * row + srcStride1[0] * (index + 1)];
                    s1[2] = src1[srcStride1[1] * row + srcStride1[0] * (index + 2)];
                    s1[3] = src1[srcStride1[1] * row + srcStride1[0] * (index + 3)];

                    s2[0] = src2[srcStride2[1] * (index + 0) + srcStride2[0] * (col+k)];
                    s2[1] = src2[srcStride2[1] * (index + 1) + srcStride2[0] * (col+k)];
                    s2[2] = src2[srcStride2[1] * (index + 2) + srcStride2[0] * (col+k)];
                    s2[3] = src2[srcStride2[1] * (index + 3) + srcStride2[0] * (col+k)];

                    int32x4_t s132x4 = vld1q_s32(s1);
                    int32x4_t s232x4 = vld1q_s32(s2);
                    int32x4_t t32x4 = vmulq_s32(s132x4, s232x4);
                    vst1q_s32(t, t32x4);
                    d[k] += t[0] + t[1] + t[2] + t[3];
                }
            }
            for (index = dim1_multiple4; index < dim1[0]; index++) 
            {

                s2[0] = src2[srcStride2[1] * index + srcStride2[0] * col];
                s2[1] = src2[srcStride2[1] * index + srcStride2[0] * (col + 1)];
                s2[2] = src2[srcStride2[1] * index + srcStride2[0] * (col + 2)];
                s2[3] = src2[srcStride2[1] * index + srcStride2[0] * (col + 3)];

                int32x4_t s132x4 = vdupq_n_s32(src1[srcStride1[1] * row + srcStride1[0] * index]);
                int32x4_t s232x4 = vld1q_s32(s2);
                int32x4_t d32x4 = vld1q_s32(d);

                int32x4_t t32x4 = vmulq_s32(s132x4, s232x4);
                d32x4 = vaddq_s32(d32x4, t32x4);
                vst1q_s32(d, d32x4);
            }
            if (NULL != src3) 
            {
                s3[0] = src3[srcStride3[1] * row + srcStride3[0] * (col + 0)];
                s3[1] = src3[srcStride3[1] * row + srcStride3[0] * (col + 1)];
                s3[2] = src3[srcStride3[1] * row + srcStride3[0] * (col + 2)];
                s3[3] = src3[srcStride3[1] * row + srcStride3[0] * (col + 3)];

                int32x4_t d32x4 = vld1q_s32(d);
                int32x4_t s332x4 = vld1q_s32(s3);
                d32x4 = vaddq_s32(d32x4, s332x4);
                vst1q_s32(d, d32x4);
            }

            dst[dstStride[1] * row + dstStride[0] * (col+0)] = CLAMP(d[0], INT8_MIN, INT8_MAX);
            dst[dstStride[1] * row + dstStride[0] * (col+1)] = CLAMP(d[1], INT8_MIN, INT8_MAX);
            dst[dstStride[1] * row + dstStride[0] * (col+2)] = CLAMP(d[2], INT8_MIN, INT8_MAX);
            dst[dstStride[1] * row + dstStride[0] * (col+3)] = CLAMP(d[3], INT8_MIN, INT8_MAX);
        }

        for (col = dim2_multiple4; col < dim2[0]; col++) 
        {
            sum = 0;       
            for (index = 0; index < dim1_multiple4; index+=4) 
            {
                s1[0] = src1[srcStride1[1] * row + srcStride1[0] * (index + 0)];
                s1[1] = src1[srcStride1[1] * row + srcStride1[0] * (index + 1)];
                s1[2] = src1[srcStride1[1] * row + srcStride1[0] * (index + 2)];
                s1[3] = src1[srcStride1[1] * row + srcStride1[0] * (index + 3)];

                s2[0] = src2[srcStride2[1] * (index + 0) + srcStride2[0] * col];
                s2[1] = src2[srcStride2[1] * (index + 1) + srcStride2[0] * col];
                s2[2] = src2[srcStride2[1] * (index + 2) + srcStride2[0] * col];
                s2[3] = src2[srcStride2[1] * (index + 3) + srcStride2[0] * col];

                int32x4_t s132x4 = vld1q_s32(s1);
                int32x4_t s232x4 = vld1q_s32(s2);
                int32x4_t t32x4 = vmulq_s32(s132x4, s232x4);
                vst1q_s32(t, t32x4);
                sum += t[0] + t[1] + t[2] + t[3];
            }
            for (index = dim1_multiple4; index < dim1[0]; index++) 
            {
                sum += src1[srcStride1[1] * row + srcStride1[0] * index] * src2[srcStride2[1] * index + srcStride2[0] * col];
            }
            if (NULL != src3) 
            {
                sum += src3[srcStride3[1] * row + srcStride3[0] * col];
            }

            dst[dstStride[1] * row + dstStride[0] * col] = CLAMP(sum, INT8_MIN, INT8_MAX);
        }
    }
}

static void TensorMultiplyMatrix_S16(const vx_int16 *src1, const vx_uint32 *dim1, const vx_uint32 *srcStride1, const vx_int16 *src2, const vx_uint32 *dim2, const vx_uint32 *srcStride2, const vx_int16 *src3, const vx_uint32 *srcStride3, const vx_uint32 *dstStride, vx_int16 *dst) 
{
    vx_uint32 row, col, index;
    vx_uint32 dim1_multiple4 = dim1[0] - dim1[0] % 4;
    vx_uint32 dim2_multiple4 = dim2[0] - dim2[0] % 4;

    vx_int32 sum, d[4], s1[4], s2[4], s3[4], t[4];

    for (row = 0; row < dim1[1]; row++) 
    {
        for (col = 0; col < dim2_multiple4; col+=4) 
        {                  
            for(vx_uint32 k = 0; k < 4; k++)
            {
                d[k] = 0;
                for (index = 0; index < dim1_multiple4; index+=4) 
                {
                    s1[0] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+0)));
                    s1[1] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+1)));
                    s1[2] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+2)));
                    s1[3] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+3)));

                    s2[0] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+0) + srcStride2[0] * (col+k)));
                    s2[1] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+1) + srcStride2[0] * (col+k)));
                    s2[2] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+2) + srcStride2[0] * (col+k)));
                    s2[3] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+3) + srcStride2[0] * (col+k)));


                    int32x4_t s132x4 = vld1q_s32(s1);
                    int32x4_t s232x4 = vld1q_s32(s2);
                    int32x4_t t32x4 = vmulq_s32(s132x4, s232x4);
                    vst1q_s32(t, t32x4);
                    d[k] += t[0] + t[1] + t[2] + t[3];
                }
            }
            for (index = dim1_multiple4; index < dim1[0]; index++) 
            {
                s2[0] =  (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * index + srcStride2[0] * (col+0)));
                s2[1] =  (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * index + srcStride2[0] * (col+1)));
                s2[2] =  (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * index + srcStride2[0] * (col+2)));
                s2[3] =  (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * index + srcStride2[0] * (col+3)));

                int32x4_t s132x4 = vdupq_n_s32((*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * index)));
                int32x4_t s232x4 = vld1q_s32(s2);
                int32x4_t d32x4 = vld1q_s32(d);

                int32x4_t t32x4 = vmulq_s32(s132x4, s232x4);
                d32x4 = vaddq_s32(d32x4, t32x4);
                vst1q_s32(d, d32x4);

            }
            if (NULL != src3) 
            {
                s3[0] = (*(vx_int16 *)((vx_int8 *)src3 + srcStride3[1] * row + srcStride3[0] * (col+0))) * Q78_SCALE;
                s3[1] = (*(vx_int16 *)((vx_int8 *)src3 + srcStride3[1] * row + srcStride3[0] * (col+1))) * Q78_SCALE;
                s3[2] = (*(vx_int16 *)((vx_int8 *)src3 + srcStride3[1] * row + srcStride3[0] * (col+2))) * Q78_SCALE;
                s3[3] = (*(vx_int16 *)((vx_int8 *)src3 + srcStride3[1] * row + srcStride3[0] * (col+3))) * Q78_SCALE;

                int32x4_t d32x4 = vld1q_s32(d);
                int32x4_t s332x4 = vld1q_s32(s3);
                d32x4 = vaddq_s32(d32x4, s332x4);
                vst1q_s32(d, d32x4);
            }
            *(vx_int16 *)((vx_int8 *)dst + dstStride[1] * row + dstStride[0] * (col+0)) = CLAMP((d[0] / Q78_SCALE), INT16_MIN, INT16_MAX);
            *(vx_int16 *)((vx_int8 *)dst + dstStride[1] * row + dstStride[0] * (col+1)) = CLAMP((d[1] / Q78_SCALE), INT16_MIN, INT16_MAX);
            *(vx_int16 *)((vx_int8 *)dst + dstStride[1] * row + dstStride[0] * (col+2)) = CLAMP((d[2] / Q78_SCALE), INT16_MIN, INT16_MAX);
            *(vx_int16 *)((vx_int8 *)dst + dstStride[1] * row + dstStride[0] * (col+3)) = CLAMP((d[3] / Q78_SCALE), INT16_MIN, INT16_MAX);
        }

        for (col = dim2_multiple4; col < dim2[0]; col++) 
        {
            sum = 0;
            for (index = 0; index < dim1_multiple4; index+=4) 
            {
                s1[0] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+0)));
                s1[1] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+1)));
                s1[2] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+2)));
                s1[3] = (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * (index+3)));

                s2[0] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+0) + srcStride2[0] * col));
                s2[1] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+1) + srcStride2[0] * col));
                s2[2] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+2) + srcStride2[0] * col));
                s2[3] = (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * (index+3) + srcStride2[0] * col));

                int32x4_t s132x4 = vld1q_s32(s1);
                int32x4_t s232x4 = vld1q_s32(s2);
                int32x4_t t32x4 = vmulq_s32(s132x4, s232x4);
                vst1q_s32(t, t32x4);
                sum += t[0] + t[1] + t[2] + t[3];
            }

            for (index = dim1_multiple4; index < dim1[0]; index++) 
            {
                sum += (*(vx_int16 *)((vx_int8 *)src1 + srcStride1[1] * row + srcStride1[0] * index)) *
                    (*(vx_int16 *)((vx_int8 *)src2 + srcStride2[1] * index + srcStride2[0] * col));
            }
            if (NULL != src3) 
            {
                sum += (*(vx_int16 *)((vx_int8 *)src3 + srcStride3[1] * row + srcStride3[0] * col)) * Q78_SCALE;
            }

            *(vx_int16 *)((vx_int8 *)dst + dstStride[1] * row + dstStride[0] * col) = CLAMP((sum / Q78_SCALE), INT16_MIN, INT16_MAX);
        }
    }
}


void Multiply2DMatrixesImpl(
        const void* src1, const vx_size* src1_strides,
        const vx_size* dims1,
        const void* src2, const vx_size* src2_strides,
        const vx_size* dims2,
        const void* src3, const vx_size* src3_strides,
        void* dst, const vx_size* dst_strides,
        vx_enum type)
{
    assert(dims1[0] == dims2[1]);

    switch (type)
    {
    case VX_TYPE_INT8: TensorMultiplyMatrix_S8((vx_uint8 *)src1, dims1, src1_strides, (vx_uint8 *)src2, dims2, src2_strides, 
                        (vx_uint8 *)src3, src3_strides, dst_strides, (vx_uint8 *)dst);
                        break;

    case VX_TYPE_UINT8: TensorMultiplyMatrix_U8((vx_int8 *)src1, dims1, src1_strides, (vx_int8 *)src2, dims2, src2_strides, 
                        (vx_int8 *)src3, src3_strides, dst_strides, (vx_int8 *)dst);
                        break;

    case VX_TYPE_INT16: TensorMultiplyMatrix_S16((vx_int16 *)src1, dims1, src1_strides, (vx_int16 *)src2, dims2, src2_strides, 
                        (vx_int16 *)src3, src3_strides, dst_strides, (vx_int16 *)dst);
                        break;

    default: assert(0); 
                        break;
    }
}

