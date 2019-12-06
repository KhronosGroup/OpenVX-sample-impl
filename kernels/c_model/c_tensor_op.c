#include <c_model.h>
#include "conversion_utils.h"
#include "tensor_utils.h"

#include <stdio.h>
#include <stdlib.h>

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
//static C_KERNEL_INLINE size_t getFlatTensorByteOffsetWithBroadcast(
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

static C_KERNEL_INLINE size_t getBaseTypeSize(enum TensorCFmt fmt)
{
    switch(fmt)
    {
    case TENSOR_C_FMT_Q78: return sizeof(int16_t);
    case TENSOR_C_FMT_U8: return sizeof(uint8_t);
    case TENSOR_C_FMT_S8: return sizeof(int8_t);
    default: assert(0); return sizeof(uint8_t);
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
        VX_TYPE_ in0_val = *(VX_TYPE_ *)((char *)input0_ptr + in0_byte_offset); \
        VX_TYPE_ in1_val = *(VX_TYPE_ *)((char *)input1_ptr + in1_byte_offset); \
        VX_TYPE_ * res_ptr = (VX_TYPE_ *)((char *)output_ptr + out_byte_offset); \
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
        VX_TYPE_ in0_val = *(VX_TYPE_ *)((char *)input0_ptr + in0_byte_offset); \
        VX_TYPE_ in1_val = *(VX_TYPE_ *)((char *)input1_ptr + in1_byte_offset); \
        VX_TYPE_ * res_ptr = (VX_TYPE_ *)((char *)output_ptr + out_byte_offset); \
        \
        do { EXPR_ } while(0); \
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
                if (wrap) { OP_TEMPLATE(int16_t, { *res_ptr = (int16_t)(in0_val + in1_val); }); }       //TODO: cast issue?
                else { OP_TEMPLATE(int16_t, { *res_ptr = conversion_24_8(in0_val + in1_val); }); }
                break;
            case ELEMENTWISE_TENSOR_SUB:
                if (wrap) { OP_TEMPLATE(int16_t, { *res_ptr = (int16_t)(in0_val - in1_val); }); }       //TODO: cast issue?
                else { OP_TEMPLATE(int16_t, { *res_ptr = conversion_24_8(in0_val - in1_val); }); }
                break;
            case ELEMENTWISE_TENSOR_MUL:
                {
                    double new_scale = (double)scale / (1 << Q78_FIXED_POINT_POSITION);

                    if (wrap)
                    {
                        if (to_ne) { OP_TEMPLATE(vx_int16, { *res_ptr = (int16_t)nearbyint(in0_val * in1_val * new_scale); }); }    //TODO: cast issue?
                        else { OP_TEMPLATE(vx_int16, { *res_ptr = (int16_t)trunc(in0_val * in1_val * new_scale); }); }              //TODO: cast issue?
                    }
                    else  // using "uint32_t" will fail the CTS test on Rasberry Pi
                    {
                        if (to_ne) { OP_TEMPLATE(vx_int16, { *res_ptr = (vx_int16)conversion_24_8(nearbyint(in0_val * in1_val * new_scale)); }); }
                        else { OP_TEMPLATE(vx_int16, { *res_ptr = (vx_int16)conversion_24_8(trunc(in0_val * in1_val * new_scale)); }); }
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
                if (wrap) { OP_TEMPLATE(uint8_t, { *res_ptr = in0_val + in1_val; }); }
                else { OP_TEMPLATE(uint8_t, { *res_ptr = CLAMP(in0_val + in1_val, 0, UINT8_MAX); }); }
                break;
            case ELEMENTWISE_TENSOR_SUB:
                if (wrap) { OP_TEMPLATE(uint8_t, { *res_ptr = in0_val - in1_val; }); }
                else { OP_TEMPLATE(uint8_t, { *res_ptr = CLAMP(in0_val - in1_val, 0, UINT8_MAX); }); }
                break;
            case ELEMENTWISE_TENSOR_MUL:
                if (wrap)
                {
                    if (to_ne) { OP_TEMPLATE(uint8_t, { *res_ptr = (uint8_t)nearbyint(in0_val * in1_val * scale); }); }
                    else { OP_TEMPLATE(uint8_t, { *res_ptr = (uint8_t)trunc(in0_val * in1_val * scale); }); }
                }
                else
                {
                    if (to_ne) { OP_TEMPLATE(uint8_t, { *res_ptr = (uint8_t)CLAMP(nearbyint(in0_val * in1_val * scale), 0, UINT8_MAX); }); }
                    else { OP_TEMPLATE(uint8_t, { *res_ptr = (uint8_t) CLAMP(trunc(in0_val * in1_val * scale), 0, UINT8_MAX); }); }
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
                if (wrap) { OP_TEMPLATE(int8_t, { *res_ptr = (int8_t)(in0_val + in1_val); }); }         //TODO: cast issue?
                else { OP_TEMPLATE(int8_t, { *res_ptr = CLAMP(in0_val + in1_val, INT8_MIN, INT8_MAX); }); }
                break;
            case ELEMENTWISE_TENSOR_SUB:
                if (wrap) { OP_TEMPLATE(int8_t, { *res_ptr = (int8_t)(in0_val - in1_val); }); }         //TODO: cast issue?
                else { OP_TEMPLATE(int8_t, { *res_ptr = CLAMP(in0_val - in1_val, INT8_MIN, INT8_MAX); }); }
                break;
            case ELEMENTWISE_TENSOR_MUL:
                if (wrap)
                {
                    if (to_ne) { OP_TEMPLATE(int8_t, { *res_ptr = (int8_t)nearbyint(in0_val * in1_val * scale); }); }   //TODO: cast issue?
                    else { OP_TEMPLATE(int8_t, { *res_ptr = (int8_t)trunc(in0_val * in1_val * scale); }); }             //TODO: cast issue?
                }
                else
                {
                    if (to_ne) { OP_TEMPLATE(int8_t, { *res_ptr = (int8_t)CLAMP(nearbyint(in0_val * in1_val * scale), INT8_MIN, INT8_MAX); }); }
                    else { OP_TEMPLATE(int8_t, { *res_ptr = (int8_t)CLAMP(trunc(in0_val * in1_val * scale), INT8_MIN, INT8_MAX); }); }
                }
                break;
            default: assert(0);
            }
        }
        break;
    default: assert(0);
    }
#undef OP_TEMPLATE
}

static C_KERNEL_INLINE void singleValueDepthConvert(
        const void * in, enum TensorCFmt src_fmt,
        bool wrap,
        float scale,
        float offset,
        /*OUT*/ void * out, enum TensorCFmt dst_fmt)
{
    float tmp;

    switch(src_fmt)
    {
    case TENSOR_C_FMT_Q78:
        tmp = *(int16_t *)in;
        tmp /= 1 << Q78_FIXED_POINT_POSITION;
        break;
    case TENSOR_C_FMT_U8: tmp = *(uint8_t *)in; break;
    case TENSOR_C_FMT_S8: tmp = *(int8_t *)in; break;
    default: assert(0);
    }

    tmp = (tmp - offset) * scale;

    switch(dst_fmt)
    {
    case TENSOR_C_FMT_Q78:
        tmp *= 1 << Q78_FIXED_POINT_POSITION;
        *(int16_t *)out = wrap ? (int16_t)tmp : (int16_t) CLAMP(tmp, INT16_MIN, INT16_MAX);  //TODO: cast issue?
        break;
    case TENSOR_C_FMT_U8:
        *(uint8_t *)out = wrap ? (uint8_t)tmp : (uint8_t)CLAMP(tmp, 0, UINT8_MAX);           // CLAMP not needed
        break;
    case TENSOR_C_FMT_S8:
        *(int8_t *)out = wrap ? (int8_t)tmp : (int8_t)CLAMP(tmp, INT8_MIN, INT8_MAX);       //TODO: cast issue?
        break;
    default: assert(0);
    }
}

void TensorConvertDepthKernelImpl(
        const void * input_ptr, tensor_desc_t input,
        enum TensorCFmt src_fmt,
        enum TensorCFmt dst_fmt,
//        vx_convert_policy_e convert_policy,
        bool wrap,  // true for wrap, sat otherwise
        float norm,
        float offset,
        void * output_ptr, tensor_desc_t output)
{
    
    assert(input.dim_num > 0 && input.dim_num <= MAX_NUM_OF_DIMENSIONS);
    assert(output.dim_num == input.dim_num);

    for (size_t i = 0; i < input.dim_num; ++i) assert(output.dims[i] == input.dims[i]);

    // Since we calc offsets manually and cast to (int16_t *), we expect the-
    // alignment to be correct already.
    {
        const size_t input_base_type_size = getBaseTypeSize(src_fmt);
        const size_t output_base_type_size = getBaseTypeSize(dst_fmt);

        for (size_t i = 0; i < input.dim_num; ++i)
        {
            assert(input.strides[i] % input_base_type_size == 0);
            assert(output.strides[i] % output_base_type_size == 0);
        }
    }

    const float scale = 1.f / norm;

#if SUPPORT_ARBITRARY_DIM_NUM
    const size_t count = ComputeNumberOfElements(input.dims, input.dim_num);

    for (size_t i = 0; i < count; ++i)
    {
        size_t input_byte_offset = ComputeGlobalPositionsFromIndex(i, input.dims, input.strides, input.dim_num, &input_byte_offset);
        size_t output_byte_offset = ComputeGlobalPositionsFromIndex(i, output.dims, output.strides, output.dim_num, &output_byte_offset);

        const void * in = (char *)input_ptr + input_byte_offset;
        const void * out = (char *)output_ptr + output_byte_offset;

        singleValueDepthConvert(in, src_fmt, wrap, scale, offset, out, dst_fmt);
    }
#else
    const size_t output_dim0 = output.dims[0];
    const size_t output_dim1 = output.dim_num > 1 ? output.dims[1]: 1;
    const size_t output_dim2 = output.dim_num > 2 ? output.dims[2]: 1;
    const size_t output_dim3 = output.dim_num > 3 ? output.dims[3]: 1;
    const size_t output_dim4 = output.dim_num > 4 ? output.dims[4]: 1;
    const size_t output_dim5 = output.dim_num > 5 ? output.dims[5]: 1;

    for (size_t i5 = 0; i5 < output_dim5; ++i5)
    for (size_t i4 = 0; i4 < output_dim4; ++i4)
    for (size_t i3 = 0; i3 < output_dim3; ++i3)
    for (size_t i2 = 0; i2 < output_dim2; ++i2)
    for (size_t i1 = 0; i1 < output_dim1; ++i1)
    for (size_t i0 = 0; i0 < output_dim0; ++i0)
    { 
        const size_t input_byte_offset =
            input.strides[5] * i5 + input.strides[4] * i4 + input.strides[3] * i3 +
            input.strides[2] * i2 + input.strides[1] * i1 + input.strides[0] * i0;
        const size_t output_byte_offset =
            output.strides[5] * i5 + output.strides[4] * i4 + output.strides[3] * i3 +
            output.strides[2] * i2 + output.strides[1] * i1 + output.strides[0] * i0;

        const void * in = (char *)input_ptr + input_byte_offset;
        void * out = (char *)output_ptr + output_byte_offset;

        singleValueDepthConvert(in, src_fmt, wrap, scale, offset, out, dst_fmt);
    }
#endif
}
