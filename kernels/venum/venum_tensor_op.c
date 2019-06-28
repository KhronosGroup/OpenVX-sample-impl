#include <venum.h>
#include "conversion_utils.h"
#include "tensor_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <arm_neon.h>
#include <VX/vx_types.h>
#include <assert.h>


// The following example supports both an arbitrary dim num and a somewhat,
// hopefully faster, hardcoded amount of loops to MAX_NUM_OF_DIMENSIONS.
// Set the following to 0 for the hardcoded loop count.
#define SUPPORT_ARBITRARY_DIM_NUM 0

#define MAX_NUM_OF_DIMENSIONS   6

#define Q78_FIXED_POINT_POSITION 8
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define CLAMP(val, lower, upper) MAX((lower), MIN((val), (upper)))


////TODO: move this together with ComputeGlobalPositionsFromIndex
////TODO: does it have to return byte offset?...
//static inline size_t getFlatTensorByteOffsetWithBroadcast(
//        size_t index,
//        vx_size dim_num,
//        const vx_size * in_dims,
//        const vx_size * in_strides,
//        const vx_size * out_dims)
//{
//    size_t res = 0;
//
//    for (vx_size d = 0; d < dim_num; ++d)
//    {
//#if 1   // support broadcast
////        assert (out_dims[d]);
////        assert (in_dims[d] == 1 || in_dims[d] == out_dims[d]);
//
//        if (in_dims[d] == out_dims[d])
//            res += in_strides[d] * (index % out_dims[d]);
//
//        index /= out_dims[d];
//#else   // ignore broadcast (possible out-of-bounds access if broadcast's present)
//        res += in_strides[d] * (index % in_dims[d]);
//        index /= in_dims[d];
//#endif
//    }
//
//    return res;
//}

static inline size_t getBaseTypeSize(enum TensorCFmt fmt)
{
    switch(fmt)
    {
    case TENSOR_C_FMT_Q78: return sizeof(vx_int16);
    case TENSOR_C_FMT_U8: return sizeof(vx_uint8);
    case TENSOR_C_FMT_S8: return sizeof(vx_int8);
    default: assert(0); return sizeof(vx_uint8);
    }
}

void ElementwiseTensorOpImpl(
        enum ElementwiseTensorMathOp op,
        enum TensorCFmt fmt,
        const void * input0_ptr, tensor_desc_t input0,
        const void * input1_ptr, tensor_desc_t input1,
        float scale,
        bool wrap,  // true for wrap, sat otherwise
        bool to_ne,  // true for to_ne, to_zero, otherwise (only usef for MUL)
        void * output_ptr, tensor_desc_t output)
{

    assert (input0.dim_num > 0);
    assert (input1.dim_num == input0.dim_num);
    assert (output.dim_num == input0.dim_num);

    for (size_t i = 0; i < input0.dim_num; ++i)
    {
        assert (output.dims[i] == input0.dims[i] || input0.dims[i] == 1);
        assert (output.dims[i] == input1.dims[i] || input1.dims[i] == 1);

        //Note: We also support here having both inputs dim equal to "1" and
        //      the output being something else, even though the spec doesn't
        //      require it. Implementations are free to additionally limit
        //      their support in the following manner,
        // assert(output.dims[i] == MAX(input0.dims[i], input1.dims[i]));


    }

    // Since we calc offsets manually and cast to (int16_t *), we expect the-
    // alignment to be correct already.
    {
        const size_t base_type_size = getBaseTypeSize(fmt);

        for (size_t i = 0; i < input0.dim_num; ++i)
        {
            assert(input0.strides[i] % base_type_size == 0);
            assert(input1.strides[i] % base_type_size == 0);
            assert(output.strides[i] % base_type_size == 0);
        }
    }

#if SUPPORT_ARBITRARY_DIM_NUM
    vx_size out_size = ComputeNumberOfElements(output.dims, output.dim_num);

#define OP_TEMPLATE(VX_TYPE_, EXPR_) \
    for (vx_size i = 0; i < out_size; ++i) \
    { \
        vx_size in0_byte_offset = getFlatTensorByteOffsetWithBroadcast(i, input0.dim_num, input0.dims, input0.strides, output.dims); \
        vx_size in1_byte_offset = getFlatTensorByteOffsetWithBroadcast(i, input1.dim_num, input1.dims, input1.strides, output.dims); \
        vx_size out_byte_offset = ComputeGlobalPositionsFromIndex(i, output.dims, output.strides, output.dim_num, &out_byte_offset); \
        \
        VX_TYPE_ in0_val = *(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset); \
        VX_TYPE_ in1_val = *(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset); \
        VX_TYPE_ * res_ptr = (VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset); \
        \
        do { EXPR_ } while(0); \
    }

#else   // SUPPORT_ARBITRARY_DIM_NUM

    const size_t output_dim0 = output.dims[0];
    const size_t output_dim1 = output.dim_num > 1 ? output.dims[1]: 1;
    const size_t output_dim2 = output.dim_num > 2 ? output.dims[2]: 1;
    const size_t output_dim3 = output.dim_num > 3 ? output.dims[3]: 1;
    const size_t output_dim4 = output.dim_num > 4 ? output.dims[4]: 1;
    const size_t output_dim5 = output.dim_num > 5 ? output.dims[5]: 1;

    // Note: It make make sense to seperate the cases per dim_num,
    //       to avoid repeated by 0 multiplication in the offset calculation.

    // Instead of using something like
    // input_byte_offset == ... + (input0.dims[3] > 1 ? i3 * input0.strides[3] : 0) + ...
    // We create an extra array of strides containning 0s if the input dim is
    // being broadcasted here, to avoid the extra contionals in the loop
    size_t in0_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };
    size_t in1_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };
    for (size_t i = 0; i < input0.dim_num; ++i)
    {
        if (input0.dims[i] > 1) in0_strides[i] = input0.strides[i];
        if (input1.dims[i] > 1) in1_strides[i] = input1.strides[i];
    }


#define OP_TEMPLATE(VX_TYPE_, EXPR_) \
    for (vx_size i5 = 0; i5 < output_dim5; ++i5) \
    for (vx_size i4 = 0; i4 < output_dim4; ++i4) \
    for (vx_size i3 = 0; i3 < output_dim3; ++i3) \
    for (vx_size i2 = 0; i2 < output_dim2; ++i2) \
    for (vx_size i1 = 0; i1 < output_dim1; ++i1) \
    for (vx_size i0 = 0; i0 < output_dim0; ++i0) \
    { \
        vx_size in0_byte_offset = \
            in0_strides[5] * i5 + in0_strides[4] * i4 + in0_strides[3] * i3 + \
            in0_strides[2] * i2 + in0_strides[1] * i1 + in0_strides[0] * i0; \
        vx_size in1_byte_offset = \
            in1_strides[5] * i5 + in1_strides[4] * i4 + in1_strides[3] * i3 + \
            in1_strides[2] * i2 + in1_strides[1] * i1 + in1_strides[0] * i0; \
        vx_size out_byte_offset = \
            output.strides[5] * i5 + output.strides[4] * i4 + output.strides[3] * i3 + \
            output.strides[2] * i2 + output.strides[1] * i1 + output.strides[0] * i0; \
        \
        VX_TYPE_ in0_val = *(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset); \
        VX_TYPE_ in1_val = *(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset); \
        VX_TYPE_ * res_ptr = (VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset); \
        \
        do { EXPR_ } while(0); \
    }

#define VENUM_ADDSUB_TEMPLATE(NEON_TYPE_, NEON_EXPO_, NEON_OP_, NEON_DUP_, NEON_LD_, NEON_ST_, VX_TYPE_, EXPR_) \
    vx_size wDim0 = (output_dim0 >> NEON_EXPO_) << NEON_EXPO_; \
    vx_size wStep = 1 << NEON_EXPO_; \
    vx_size i0; \
    for (vx_size i5 = 0; i5 < output_dim5; ++i5) \
    for (vx_size i4 = 0; i4 < output_dim4; ++i4) \
    for (vx_size i3 = 0; i3 < output_dim3; ++i3) \
    for (vx_size i2 = 0; i2 < output_dim2; ++i2) \
    for (vx_size i1 = 0; i1 < output_dim1; ++i1) \
    { \
        vx_size in0_byte_offset = \
            in0_strides[5] * i5 + in0_strides[4] * i4 + in0_strides[3] * i3 + \
            in0_strides[2] * i2 + in0_strides[1] * i1; \
        vx_size in1_byte_offset = \
            in1_strides[5] * i5 + in1_strides[4] * i4 + in1_strides[3] * i3 + \
            in1_strides[2] * i2 + in1_strides[1] * i1; \
        vx_size out_byte_offset = \
            output.strides[5] * i5 + output.strides[4] * i4 + output.strides[3] * i3 + \
            output.strides[2] * i2 + output.strides[1] * i1; \
        for (i0 = 0; i0 < wDim0; i0 += wStep) \
        { \
            NEON_TYPE_ vIn0, vIn1; \
            if (0 != in0_strides[0]) \
            { \
                vIn0 = NEON_LD_((VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset + in0_strides[0] * i0)); \
            } \
            else \
            { \
                vIn0 = NEON_DUP_(*(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset)); \
            } \
            if (0 != in1_strides[0]) \
            { \
                vIn1 = NEON_LD_((VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset + in1_strides[0] * i0)); \
            } \
            else \
            { \
                vIn1 = NEON_DUP_(*(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset)); \
            } \
            NEON_TYPE_ vRet = NEON_OP_(vIn0, vIn1); \
            NEON_ST_((VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset + output.strides[0] * i0), vRet); \
        } \
        for (i0 = wDim0; i0 < output_dim0; ++i0) \
        { \
            VX_TYPE_ in0_val = *(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset + in0_strides[0] * i0); \
            VX_TYPE_ in1_val = *(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset + in1_strides[0] * i0); \
            VX_TYPE_ * res_ptr = (VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset + output.strides[0] * i0); \
            \
            do { EXPR_ } while(0); \
        } \
    }

#define VENUM_8MUL_TEMPLATE(NEON_TYPE_, NEON_EXPO_, NEON_OP_, NEON_DUP_, NEON_LD_, NEON_ST_, \
                           NEON_CVT_LO_EXPR0_, NEON_CVT_HI_EXPR0_, \
                           NEON_CVT_LO_EXPR1_, NEON_CVT_HI_EXPR1_, NEON_CVT_RET_, \
                           MUL_SCALE_, BOOL_NEARBYINT_,VX_TYPE_, EXPR_) \
    vx_size wDim0 = (output_dim0 >> NEON_EXPO_) << NEON_EXPO_; \
    vx_size wStep = 1 << NEON_EXPO_; \
    vx_size i0; \
    for (vx_size i5 = 0; i5 < output_dim5; ++i5) \
    for (vx_size i4 = 0; i4 < output_dim4; ++i4) \
    for (vx_size i3 = 0; i3 < output_dim3; ++i3) \
    for (vx_size i2 = 0; i2 < output_dim2; ++i2) \
    for (vx_size i1 = 0; i1 < output_dim1; ++i1) \
    { \
        for (i0 = 0; i0 < wDim0; i0 += wStep) \
        { \
            vx_size in0_byte_offset = \
                in0_strides[5] * i5 + in0_strides[4] * i4 + in0_strides[3] * i3 + \
                in0_strides[2] * i2 + in0_strides[1] * i1 + in0_strides[0] * i0; \
            vx_size in1_byte_offset = \
                in1_strides[5] * i5 + in1_strides[4] * i4 + in1_strides[3] * i3 + \
                in1_strides[2] * i2 + in1_strides[1] * i1 + in1_strides[0] * i0; \
            vx_size out_byte_offset = \
                output.strides[5] * i5 + output.strides[4] * i4 + output.strides[3] * i3 + \
                output.strides[2] * i2 + output.strides[1] * i1 + output.strides[0] * i0; \
            \
            NEON_TYPE_ vIn0, vIn1; \
            if (0 != in0_strides[0]) \
            { \
                vIn0 = NEON_LD_((VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset)); \
            } \
            else \
            { \
                vIn0 = NEON_DUP_(*(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset)); \
            } \
            if (0 != in1_strides[0]) \
            { \
                vIn1 = NEON_LD_((VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset)); \
            } \
            else \
            { \
                vIn1 = NEON_DUP_(*(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset)); \
            } \
            float32x4_t vIn0_lo = NEON_CVT_LO_EXPR0_; \
            float32x4_t vIn0_hi = NEON_CVT_HI_EXPR0_; \
            float32x4_t vIn1_lo = NEON_CVT_LO_EXPR1_; \
            float32x4_t vIn1_hi = NEON_CVT_HI_EXPR1_; \
            float32x4_t vRet_lo = NEON_OP_(vIn0_lo, vIn1_lo); \
            float32x4_t vRet_hi = NEON_OP_(vIn0_hi, vIn1_hi); \
            vRet_lo = NEON_OP_(vRet_lo, MUL_SCALE_); \
            vRet_hi = NEON_OP_(vRet_hi, MUL_SCALE_); \
            if (BOOL_NEARBYINT_) \
            { \
                vRet_lo = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_lo, 0)), vRet_lo, 0); \
                vRet_lo = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_lo, 1)), vRet_lo, 1); \
                vRet_lo = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_lo, 2)), vRet_lo, 2); \
                vRet_lo = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_lo, 3)), vRet_lo, 3); \
                vRet_hi = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_hi, 0)), vRet_hi, 0); \
                vRet_hi = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_hi, 1)), vRet_hi, 1); \
                vRet_hi = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_hi, 2)), vRet_hi, 2); \
                vRet_hi = vsetq_lane_f32(nearbyint(vgetq_lane_f32(vRet_hi, 3)), vRet_hi, 3); \
            } \
            NEON_TYPE_ vRet = NEON_CVT_RET_; \
            NEON_ST_((VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset), vRet); \
        } \
        for (i0 = wDim0; i0 < output_dim0; ++i0) \
        { \
            vx_size in0_byte_offset = \
                in0_strides[5] * i5 + in0_strides[4] * i4 + in0_strides[3] * i3 + \
                in0_strides[2] * i2 + in0_strides[1] * i1 + in0_strides[0] * i0; \
            vx_size in1_byte_offset = \
                in1_strides[5] * i5 + in1_strides[4] * i4 + in1_strides[3] * i3 + \
                in1_strides[2] * i2 + in1_strides[1] * i1 + in1_strides[0] * i0; \
            vx_size out_byte_offset = \
                output.strides[5] * i5 + output.strides[4] * i4 + output.strides[3] * i3 + \
                output.strides[2] * i2 + output.strides[1] * i1 + output.strides[0] * i0; \
            \
            VX_TYPE_ in0_val = *(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset); \
            VX_TYPE_ in1_val = *(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset); \
            VX_TYPE_ * res_ptr = (VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset); \
            \
            do { EXPR_ } while(0); \
        } \
    }

#define VENUM_Q78MUL_TEMPLATE(NEON_TYPE_, NEON_EXPO_, NEON_OP_, NEON_DUP_, NEON_LD_, NEON_ST_, \
                              NEON_CVT_LO_EXPR0_, NEON_CVT_HI_EXPR0_, \
                              NEON_CVT_LO_EXPR1_, NEON_CVT_HI_EXPR1_, NEON_CVT_RET_, \
                              MUL_SCALE_, BOOL_NEARBYINT_,VX_TYPE_, EXPR_) \
    vx_size wDim0 = (output_dim0 >> NEON_EXPO_) << NEON_EXPO_; \
    vx_size wStep = 1 << NEON_EXPO_; \
    vx_size i0; \
    for (vx_size i5 = 0; i5 < output_dim5; ++i5) \
    for (vx_size i4 = 0; i4 < output_dim4; ++i4) \
    for (vx_size i3 = 0; i3 < output_dim3; ++i3) \
    for (vx_size i2 = 0; i2 < output_dim2; ++i2) \
    for (vx_size i1 = 0; i1 < output_dim1; ++i1) \
    { \
        for (i0 = 0; i0 < wDim0; i0 += wStep) \
        { \
            vx_size in0_byte_offset = \
                in0_strides[5] * i5 + in0_strides[4] * i4 + in0_strides[3] * i3 + \
                in0_strides[2] * i2 + in0_strides[1] * i1 + in0_strides[0] * i0; \
            vx_size in1_byte_offset = \
                in1_strides[5] * i5 + in1_strides[4] * i4 + in1_strides[3] * i3 + \
                in1_strides[2] * i2 + in1_strides[1] * i1 + in1_strides[0] * i0; \
            vx_size out_byte_offset = \
                output.strides[5] * i5 + output.strides[4] * i4 + output.strides[3] * i3 + \
                output.strides[2] * i2 + output.strides[1] * i1 + output.strides[0] * i0; \
            \
            NEON_TYPE_ vIn0, vIn1; \
            if (0 != in0_strides[0]) \
            { \
                vIn0 = NEON_LD_((VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset)); \
            } \
            else \
            { \
                vIn0 = NEON_DUP_(*(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset)); \
            } \
            if (0 != in1_strides[0]) \
            { \
                vIn1 = NEON_LD_((VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset)); \
            } \
            else \
            { \
                vIn1 = NEON_DUP_(*(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset)); \
            } \
            int32x4_t vIn0_lo = NEON_CVT_LO_EXPR0_; \
            int32x4_t vIn0_hi = NEON_CVT_HI_EXPR0_; \
            int32x4_t vIn1_lo = NEON_CVT_LO_EXPR1_; \
            int32x4_t vIn1_hi = NEON_CVT_HI_EXPR1_; \
            int32x4_t vRet_lo = NEON_OP_(vIn0_lo, vIn1_lo); \
            int32x4_t vRet_hi = NEON_OP_(vIn0_hi, vIn1_hi); \
            if (BOOL_NEARBYINT_) \
            { \
                vRet_lo = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_lo, 0) * (double)MUL_SCALE_), vRet_lo, 0); \
                vRet_lo = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_lo, 1) * (double)MUL_SCALE_), vRet_lo, 1); \
                vRet_lo = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_lo, 2) * (double)MUL_SCALE_), vRet_lo, 2); \
                vRet_lo = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_lo, 3) * (double)MUL_SCALE_), vRet_lo, 3); \
                vRet_hi = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_hi, 0) * (double)MUL_SCALE_), vRet_hi, 0); \
                vRet_hi = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_hi, 1) * (double)MUL_SCALE_), vRet_hi, 1); \
                vRet_hi = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_hi, 2) * (double)MUL_SCALE_), vRet_hi, 2); \
                vRet_hi = vsetq_lane_s32((vx_int32)nearbyint(vgetq_lane_s32(vRet_hi, 3) * (double)MUL_SCALE_), vRet_hi, 3); \
            } \
            else \
            { \
                vRet_lo = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_lo, 0) * (double)MUL_SCALE_), vRet_lo, 0); \
                vRet_lo = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_lo, 1) * (double)MUL_SCALE_), vRet_lo, 1); \
                vRet_lo = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_lo, 2) * (double)MUL_SCALE_), vRet_lo, 2); \
                vRet_lo = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_lo, 3) * (double)MUL_SCALE_), vRet_lo, 3); \
                vRet_hi = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_hi, 0) * (double)MUL_SCALE_), vRet_hi, 0); \
                vRet_hi = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_hi, 1) * (double)MUL_SCALE_), vRet_hi, 1); \
                vRet_hi = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_hi, 2) * (double)MUL_SCALE_), vRet_hi, 2); \
                vRet_hi = vsetq_lane_s32((vx_int32)trunc(vgetq_lane_s32(vRet_hi, 3) * (double)MUL_SCALE_), vRet_hi, 3); \
            } \
            NEON_TYPE_ vRet = NEON_CVT_RET_; \
            NEON_ST_((VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset), vRet); \
        } \
        for (i0 = wDim0; i0 < output_dim0; ++i0) \
        { \
            vx_size in0_byte_offset = \
                in0_strides[5] * i5 + in0_strides[4] * i4 + in0_strides[3] * i3 + \
                in0_strides[2] * i2 + in0_strides[1] * i1 + in0_strides[0] * i0; \
            vx_size in1_byte_offset = \
                in1_strides[5] * i5 + in1_strides[4] * i4 + in1_strides[3] * i3 + \
                in1_strides[2] * i2 + in1_strides[1] * i1 + in1_strides[0] * i0; \
            vx_size out_byte_offset = \
                output.strides[5] * i5 + output.strides[4] * i4 + output.strides[3] * i3 + \
                output.strides[2] * i2 + output.strides[1] * i1 + output.strides[0] * i0; \
            \
            VX_TYPE_ in0_val = *(VX_TYPE_ *)((vx_int8 *)input0_ptr + in0_byte_offset); \
            VX_TYPE_ in1_val = *(VX_TYPE_ *)((vx_int8 *)input1_ptr + in1_byte_offset); \
            VX_TYPE_ * res_ptr = (VX_TYPE_ *)((vx_int8 *)output_ptr + out_byte_offset); \
            \
            do { EXPR_ } while(0); \
        } \
    }

#endif  // SUPPORT_ARBITRARY_DIM_NUM

    //TODO: Conversion to signed int from an out of bounds value as used in
    //      wrap, is impl-defined! This shoudl be fixed by doing it through
    //      an unsigned conversion into a union betweenthe signed and unsigned
    //      values from which the unsigned one would be read out...
    //      (Is this the correct way to do it portably in C?)
    //      (This is mostly relevant for Q78)

    switch (fmt)
    {
    case TENSOR_C_FMT_Q78:
        {
            switch (op)
            {
            case ELEMENTWISE_TENSOR_ADD:
                if (wrap)
                {
                    VENUM_ADDSUB_TEMPLATE(int16x8_t, 3, vaddq_s16, vdupq_n_s16, vld1q_s16, vst1q_s16,
                        vx_int16, { *res_ptr = (vx_int16)(in0_val + in1_val); });
                }
                else
                {
                    VENUM_ADDSUB_TEMPLATE(int16x8_t, 3, vqaddq_s16, vdupq_n_s16, vld1q_s16, vst1q_s16,
                        vx_int16, { *res_ptr = conversion_24_8(in0_val + in1_val); });
                }
                break;
            case ELEMENTWISE_TENSOR_SUB:
                if (wrap)
                {
                    VENUM_ADDSUB_TEMPLATE(int16x8_t, 3, vsubq_s16, vdupq_n_s16, vld1q_s16, vst1q_s16,
                        vx_int16, { *res_ptr = (vx_int16)(in0_val - in1_val); });
                }
                else
                {
                    VENUM_ADDSUB_TEMPLATE(int16x8_t, 3, vqsubq_s16, vdupq_n_s16, vld1q_s16, vst1q_s16,
                        vx_int16, { *res_ptr = conversion_24_8(in0_val - in1_val); });
                }
                break;
            case ELEMENTWISE_TENSOR_MUL:
                {
                    double new_scale = (double)scale / (1 << Q78_FIXED_POINT_POSITION);

                    if (wrap)
                    {
                        if (to_ne)
                        {
                            VENUM_Q78MUL_TEMPLATE(int16x8_t, 3, vmulq_s32, vdupq_n_s16, vld1q_s16, vst1q_s16,
                               vmovl_s16(vget_low_s16(vIn0)),
                               vmovl_s16(vget_high_s16(vIn0)),
                               vmovl_s16(vget_low_s16(vIn1)),
                               vmovl_s16(vget_high_s16(vIn1)),
                               vcombine_s16(vmovn_s32(vRet_lo), vmovn_s32(vRet_hi)),
                               new_scale, 1,
                               vx_int16, { *res_ptr = (vx_int16)nearbyint(in0_val * in1_val * new_scale); });
                        }
                        else
                        {
                            VENUM_Q78MUL_TEMPLATE(int16x8_t, 3, vmulq_s32, vdupq_n_s16, vld1q_s16, vst1q_s16,
                               vmovl_s16(vget_low_s16(vIn0)),
                               vmovl_s16(vget_high_s16(vIn0)),
                               vmovl_s16(vget_low_s16(vIn1)),
                               vmovl_s16(vget_high_s16(vIn1)),
                               vcombine_s16(vmovn_s32(vRet_lo), vmovn_s32(vRet_hi)),
                               new_scale, 0,
                               vx_int16, { *res_ptr = (vx_int16)trunc(in0_val * in1_val * new_scale); });
                        }
                    }
                    else
                    {
                        if (to_ne)
                        {
                            VENUM_Q78MUL_TEMPLATE(int16x8_t, 3, vmulq_s32, vdupq_n_s16, vld1q_s16, vst1q_s16,
                               vmovl_s16(vget_low_s16(vIn0)),
                               vmovl_s16(vget_high_s16(vIn0)),
                               vmovl_s16(vget_low_s16(vIn1)),
                               vmovl_s16(vget_high_s16(vIn1)),
                               vcombine_s16(vqmovn_s32(vRet_lo), vqmovn_s32(vRet_hi)),
                               new_scale, 1,
                               vx_int16, { *res_ptr = conversion_24_8(nearbyint(in0_val * in1_val * new_scale)); });
                        }
                        else
                        {
                            VENUM_Q78MUL_TEMPLATE(int16x8_t, 3, vmulq_s32, vdupq_n_s16, vld1q_s16, vst1q_s16,
                               vmovl_s16(vget_low_s16(vIn0)),
                               vmovl_s16(vget_high_s16(vIn0)),
                               vmovl_s16(vget_low_s16(vIn1)),
                               vmovl_s16(vget_high_s16(vIn1)),
                               vcombine_s16(vqmovn_s32(vRet_lo), vqmovn_s32(vRet_hi)),
                               new_scale, 0,
                               vx_int16, { *res_ptr = conversion_24_8(trunc(in0_val * in1_val * new_scale)); });
                        }
                    }
                }
                break;
            default: assert(0);
            }
        }
        break;
    case TENSOR_C_FMT_U8:
        {
            switch (op)
            {
            case ELEMENTWISE_TENSOR_ADD:
                if (wrap)
                {
                    VENUM_ADDSUB_TEMPLATE(uint8x16_t, 4, vaddq_u8, vdupq_n_u8, vld1q_u8, vst1q_u8,
                        vx_uint8, { *res_ptr = in0_val + in1_val; });
                }
                else
                {
                    VENUM_ADDSUB_TEMPLATE(uint8x16_t, 4, vqaddq_u8, vdupq_n_u8, vld1q_u8, vst1q_u8,
                        vx_uint8, { *res_ptr = CLAMP(in0_val + in1_val, 0, UINT8_MAX); });
                }
                break;
            case ELEMENTWISE_TENSOR_SUB:
                if (wrap)
                {
                    VENUM_ADDSUB_TEMPLATE(uint8x16_t, 4, vsubq_u8, vdupq_n_u8, vld1q_u8, vst1q_u8,
                        vx_uint8, { *res_ptr = in0_val - in1_val; });
                }
                else
                {
                    VENUM_ADDSUB_TEMPLATE(uint8x16_t, 4, vqsubq_u8, vdupq_n_u8, vld1q_u8, vst1q_u8,
                        vx_uint8, { *res_ptr = CLAMP(in0_val - in1_val, 0, UINT8_MAX); });
                }
                break;
            case ELEMENTWISE_TENSOR_MUL:
                if (wrap)
                {
                    if (to_ne)
                    {
                        VENUM_8MUL_TEMPLATE(uint8x8_t, 3, vmulq_f32, vdup_n_u8, vld1_u8, vst1_u8,
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn1)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn1)))),
                           vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(vRet_lo)), vmovn_u32(vcvtq_u32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 1,
                           vx_uint8, { *res_ptr = nearbyint(in0_val * in1_val * scale); });
                    }
                    else
                    {
                        VENUM_8MUL_TEMPLATE(uint8x8_t, 3, vmulq_f32, vdup_n_u8, vld1_u8, vst1_u8,
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn1)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn1)))),
                           vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(vRet_lo)), vmovn_u32(vcvtq_u32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 0,
                           vx_uint8, { *res_ptr = trunc(in0_val * in1_val * scale); });
                    }
                }
                else
                {
                    if (to_ne)
                    {
                        VENUM_8MUL_TEMPLATE(uint8x8_t, 3, vmulq_f32, vdup_n_u8, vld1_u8, vst1_u8,
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn1)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn1)))),
                           vqmovn_u16(vcombine_u16(vqmovn_u32(vcvtq_u32_f32(vRet_lo)), vqmovn_u32(vcvtq_u32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 1,
                           vx_uint8, { *res_ptr = CLAMP(nearbyint(in0_val * in1_val * scale), 0, UINT8_MAX); });
                    }
                    else
                    {
                        VENUM_8MUL_TEMPLATE(uint8x8_t, 3, vmulq_f32, vdup_n_u8, vld1_u8, vst1_u8,
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn0)))),
                           vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vIn1)))),
                           vcvtq_f32_u32(vmovl_u16(vget_high_u16(vmovl_u8(vIn1)))),
                           vqmovn_u16(vcombine_u16(vqmovn_u32(vcvtq_u32_f32(vRet_lo)), vqmovn_u32(vcvtq_u32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 0,
                           vx_uint8, { *res_ptr = CLAMP(trunc(in0_val * in1_val * scale), 0, UINT8_MAX); });
                    }
                }
                break;
            default: assert(0);
            }
        }
        break;
    case TENSOR_C_FMT_S8:
        {
            switch (op)
            {
            case ELEMENTWISE_TENSOR_ADD:
                if (wrap)
                {
                    VENUM_ADDSUB_TEMPLATE(int8x16_t, 4, vaddq_s8, vdupq_n_s8, vld1q_s8, vst1q_s8,
                        vx_int8, { *res_ptr = (vx_int8)(in0_val + in1_val); });
                }
                else
                {
                    VENUM_ADDSUB_TEMPLATE(int8x16_t, 4, vqaddq_s8, vdupq_n_s8, vld1q_s8, vst1q_s8,
                        vx_int8, { *res_ptr = CLAMP(in0_val + in1_val, INT8_MIN, INT8_MAX); });
                }
                break;
            case ELEMENTWISE_TENSOR_SUB:
                if (wrap)
                {
                    VENUM_ADDSUB_TEMPLATE(int8x16_t, 4, vsubq_s8, vdupq_n_s8, vld1q_s8, vst1q_s8,
                        vx_int8, { *res_ptr = (vx_int8)(in0_val - in1_val); });
                }
                else
                {
                    VENUM_ADDSUB_TEMPLATE(int8x16_t, 4, vqsubq_s8, vdupq_n_s8, vld1q_s8, vst1q_s8,
                        vx_int8, { *res_ptr = CLAMP(in0_val - in1_val, INT8_MIN, INT8_MAX); });
                }
                break;
            case ELEMENTWISE_TENSOR_MUL:
                if (wrap)
                {
                    if (to_ne)
                    {
                        VENUM_8MUL_TEMPLATE(int8x8_t, 3, vmulq_f32, vdup_n_s8, vld1_s8, vst1_s8,
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn1)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn1)))),
                           vmovn_s16(vcombine_s16(vmovn_s32(vcvtq_s32_f32(vRet_lo)), vmovn_s32(vcvtq_s32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 1,
                           vx_int8, { *res_ptr = (vx_int8)nearbyint(in0_val * in1_val * scale); });
                    }
                    else
                    {
                        VENUM_8MUL_TEMPLATE(int8x8_t, 3, vmulq_f32, vdup_n_s8, vld1_s8, vst1_s8,
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn1)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn1)))),
                           vmovn_s16(vcombine_s16(vmovn_s32(vcvtq_s32_f32(vRet_lo)), vmovn_s32(vcvtq_s32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 0,
                           vx_int8, { *res_ptr = (vx_int8)trunc(in0_val * in1_val * scale); });
                    }
                }
                else
                {
                    if (to_ne)
                    {
                        VENUM_8MUL_TEMPLATE(int8x8_t, 3, vmulq_f32, vdup_n_s8, vld1_s8, vst1_s8,
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn1)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn1)))),
                           vqmovn_s16(vcombine_s16(vqmovn_s32(vcvtq_s32_f32(vRet_lo)), vqmovn_s32(vcvtq_s32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 1,
                           vx_int8, { *res_ptr = CLAMP(nearbyint(in0_val * in1_val * scale), INT8_MIN, INT8_MAX); });
                    }
                    else
                    {
                        VENUM_8MUL_TEMPLATE(int8x8_t, 3, vmulq_f32, vdup_n_s8, vld1_s8, vst1_s8,
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn0)))),
                           vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vIn1)))),
                           vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vIn1)))),
                           vqmovn_s16(vcombine_s16(vqmovn_s32(vcvtq_s32_f32(vRet_lo)), vqmovn_s32(vcvtq_s32_f32(vRet_hi)))),
                           vdupq_n_f32(scale), 0,
                           vx_int8, { *res_ptr = CLAMP(trunc(in0_val * in1_val * scale), INT8_MIN, INT8_MAX); });
                    }
                }
                break;
            default: assert(0);
            }
        }
        break;
    default: assert(0);
    }
#undef OP_TEMPLATE
#undef VENUM_ADDSUB_TEMPLATE
#undef VENUM_8MUL_TEMPLATE
#undef VENUM_Q78MUL_TEMPLATE
}

