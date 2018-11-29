/*
 * c_khr_nn.c
 *
 *  Created on: Jan 15, 2017
 *      Author: vcherp
 */

#ifdef OPENVX_USE_NN

#include "c_model.h"

#include <conversion_utils.h>
#include <tensor_utils.h>

#include <VX/vx_khr_nn.h>

#include <assert.h>
#include <stdlib.h>

#define MAX_NUM_OF_DIMENSIONS   6
#define PWL_NORM_NUM_SEGMENTS   64


/****************************************************************************
 *                                                                          *
 *                              Common Utils                                *
 *                                                                          *
 ***************************************************************************/

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define CLAMP(v, lower, upper) MAX((lower), MIN((v), (upper)))

#define Q78_FIXED_POINT_POSITION 8
#define Q78_SCALE   (1 << Q78_FIXED_POINT_POSITION)
#define Q78_HALF    (1 << (Q78_FIXED_POINT_POSITION - 1))

static C_KERNEL_INLINE int_fast32_t getMinValue(enum TensorCFmt fmt)
{
    switch(fmt)
    {
        case TENSOR_C_FMT_Q78: return INT16_MIN;
        case TENSOR_C_FMT_U8: return 0;
        case TENSOR_C_FMT_S8: return INT8_MIN;
        default: assert(0); return 0;
    }
}

static C_KERNEL_INLINE int_fast32_t getMaxValue(enum TensorCFmt fmt)
{
    switch(fmt)
    {
        case TENSOR_C_FMT_Q78: return INT16_MAX;
        case TENSOR_C_FMT_U8: return UINT8_MAX;
        case TENSOR_C_FMT_S8: return INT8_MAX;
        default: assert(0); return 0;
    }
}

static int_fast32_t getSizeofType(enum TensorCFmt fmt)
{
    switch(fmt)
    {
        case TENSOR_C_FMT_Q78: return sizeof(int16_t);
        case TENSOR_C_FMT_U8: return sizeof(uint8_t);
        case TENSOR_C_FMT_S8: return sizeof(int8_t);
        default: assert(0); return 1;
    }
}

// Since we calc offsets manually and cast to ptr type, we expect the strides
// to have the correct alignment
static void assertStridesModSizeof(enum TensorCFmt fmt, tensor_desc_t td)
{
    const size_t sizeof_type = getSizeofType(fmt);

    for (size_t i = 0; i < td.dim_num; ++i)
    {
        assert(td.strides[i] % sizeof_type == 0);
    }
}

static C_KERNEL_INLINE int_fast32_t loadValueAsRawInt(enum TensorCFmt fmt, const void * ptr)
{
    switch(fmt)
    {
        case TENSOR_C_FMT_Q78: return *(int16_t*)ptr;
        case TENSOR_C_FMT_U8: return *(uint8_t*)ptr;
        case TENSOR_C_FMT_S8: return *(int8_t*)ptr;
        default: assert(0); return 0;
    }
}

static C_KERNEL_INLINE void storeRawIntValue(enum TensorCFmt fmt, int_fast32_t val, /*OUT*/ void * ptr)
{
    switch(fmt)
    {
        case TENSOR_C_FMT_Q78: *(int16_t*)ptr = val; break;
        case TENSOR_C_FMT_U8: *(uint8_t*)ptr = val; break;
        case TENSOR_C_FMT_S8: *(int8_t*)ptr = val; break;
        default: assert(0);
    }
}


static C_KERNEL_INLINE float value2Float(enum TensorCFmt fmt, int_fast32_t val)
{
    switch(fmt)
    {
        case TENSOR_C_FMT_Q78: return short2float((int16_t)val);
        case TENSOR_C_FMT_U8: return (float)val;
        case TENSOR_C_FMT_S8: return (float)val;
        default: assert(0);
    }
    return -1;
}

// Avoid impl defined behaviour when casting non representable values to signed
//TODO: is trunc indeed the type of cast the OpenVX spec demands??
static C_KERNEL_INLINE int8_t trunc_to_int8(int_fast32_t val)
{
    union { int8_t i; uint8_t u; } tmp;
    tmp.u = val;
    return tmp.i;
}

// Avoid impl defined behaviour when casting non representable values to signed
//TODO: is trunc indeed the type of cast the OpenVX spec demands??
static C_KERNEL_INLINE int16_t trunc_to_int16(int_fast32_t val)
{
    union { int16_t i; uint16_t u; } tmp;
    tmp.u = val;
    return tmp.i;
}

static C_KERNEL_INLINE int_fast32_t wrapOrSat(enum TensorCFmt fmt, int_fast32_t val, bool wrap)
{
    switch(fmt)
    {
        case TENSOR_C_FMT_Q78: return wrap? trunc_to_int16(val) : CLAMP(val, INT16_MIN, INT16_MAX);
        case TENSOR_C_FMT_U8: return wrap? (uint8_t)val : CLAMP(val, 0, UINT8_MAX);
        case TENSOR_C_FMT_S8: return wrap? trunc_to_int8(val) : CLAMP(val, INT8_MIN, INT8_MAX);
        default: assert(0); return 0;
    }
}

// Finalize the accum (sum of products) by applying rounding, norm and OF
//
// Rounding and scaling only apply to Q78, where the product results in 16
// "fractional" bits, rather than the normal 8.
static C_KERNEL_INLINE int_fast32_t applyWrapRoundingToAccum(
        enum TensorCFmt fmt, int_fast32_t val,
        bool wrap,  // true for WRAP, else SATURATE
        bool to_ne) // true for ROUND_TO_NE, else ROUND_TO_ZERO
{
    if (fmt == TENSOR_C_FMT_Q78)
    {
       if (to_ne)
       {
           val += Q78_HALF;
       }

       val /= Q78_SCALE;
    }

    return wrapOrSat(fmt, val, wrap);
}

static C_KERNEL_INLINE float unquantize(enum TensorCFmt fmt, int_fast32_t val)
{
    return fmt == TENSOR_C_FMT_Q78 ? ((float)val / Q78_SCALE) : val;
}

static C_KERNEL_INLINE int_fast32_t quantize(enum TensorCFmt fmt, float val)
{
    if (fmt == TENSOR_C_FMT_Q78) val *= Q78_SCALE;

    return wrapOrSat(fmt, val, false);
}


/****************************************************************************
 *                                                                          *
 *                              New Kernel Impls                            *
 *                                                                          *
 ***************************************************************************/

void ConvolutionKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        const void * weight_ptr, tensor_desc_t weight,
        const void * bias_ptr, tensor_desc_t bias,
        size_t pad_x, size_t pad_y,
        size_t stride_x, size_t stride_y,
        bool wrap,  // true for WRAP, else SATURATE
        bool to_ne, // true for ROUND_TO_NE, else ROUND_TO_ZERO (only used for fmt == TT_MUL)
        size_t dilation_x, size_t dilation_y,
        void * output_ptr, tensor_desc_t output)
{
    assert(fmt == TENSOR_C_FMT_Q78 || fmt == TENSOR_C_FMT_U8 || fmt == TENSOR_C_FMT_S8);

    assert(input.dim_num == 3 || input.dim_num == 4);
    assert(weight.dim_num == 4);
    assert(bias.dim_num == 0 || bias.dim_num == 1 || bias.dim_num == 3);
    assert(output.dim_num == input.dim_num);

    const size_t input_w = input.dims[0];
    const size_t input_h = input.dims[1];
    const size_t input_c = input.dims[2];
    const size_t input_b = input.dim_num > 3 ? input.dims[3] : 1;

    const size_t weight_w = weight.dims[0];
    const size_t weight_h = weight.dims[1];
    const size_t weight_ifm = weight.dims[2];
    const size_t weight_ofm = weight.dims[3];

    const bool bias_present = !!bias.dim_num;
    const bool bias_shared = bias.dim_num == 1;
    const size_t bias_w = bias.dim_num > 0 ? bias.dims[0] : 0;
    const size_t bias_h = bias.dim_num > 1 ? bias.dims[1] : 1;
    const size_t bias_ofm = bias.dim_num > 2 ? bias.dims[2] : 1;

    const size_t output_w = output.dims[0];
    const size_t output_h = output.dims[1];
    const size_t output_c = output.dims[2];
    const size_t output_b = output.dim_num > 3 ? output.dims[3] : 1;

    assert(weight_w + (weight_w - 1) * dilation_x <= input_w + 2 * pad_x);
    assert(weight_h + (weight_h - 1) * dilation_y <= input_h + 2 * pad_y);
    assert(weight_ifm == input_c);
    assert(weight_ofm == output_c);

    if (bias_shared)
    {
        assert(bias_w == weight_ofm);
    }
    else if (bias_present)
    {
        assert(bias_w == output_w);
        assert(bias_h == output_h);
        assert(bias_ofm == output_c);
    }

    assert(output_b == input_b);

    assertStridesModSizeof(fmt, input);
    assertStridesModSizeof(fmt, weight);
    assertStridesModSizeof(fmt, bias);
    assertStridesModSizeof(fmt, output);

    // Input and output pointers for the current batch being processed,
    // Note: The compiler should've been able to hoist this out... And
    // there's a bunch of other possible hoising iopportunities here.
    const char * in_b_ptr = input_ptr;
    char * out_b_ptr = output_ptr;

    for (size_t b = 0; b < output_b; ++b, in_b_ptr += input.strides[3], out_b_ptr += output.strides[3])
    for (size_t ofm = 0; ofm < output_c; ++ofm)
    for (size_t y = 0; y < output_h; ++y)
    for (size_t x = 0; x < output_w; ++x)
    {
        int32_t sum = 0;
        if (bias_present)
        {
            const size_t bias_byte_offset =
                bias_shared
                ? (bias.strides[0] * ofm)
                : (bias.strides[2] * ofm + bias.strides[1] * y + bias.strides[0] * x);

            sum = loadValueAsRawInt(fmt, (char *)bias_ptr + bias_byte_offset);
        }
        
        const size_t xx = x * stride_x;
        const size_t yy = y * stride_y;

        for (size_t ifm = 0; ifm < input_c; ++ifm)
        {
            for (size_t w_y = 0; w_y < weight_h; ++w_y)
            for (size_t w_x = 0; w_x < weight_w; ++w_x)
            {
                const size_t tmp_x = xx + w_x * (dilation_x + 1) + dilation_x;
                const size_t tmp_y = yy + w_y * (dilation_y + 1) + dilation_y;

                if (tmp_x >= pad_x && tmp_x < input_w + pad_x &&
                    tmp_y >= pad_y && tmp_y < input_h + pad_y)
                {
                    const size_t input_byte_offset =
                        input.strides[2] * ifm +
                        input.strides[1] * (tmp_y - pad_y) +
                        input.strides[0] * (tmp_x - pad_x);
                    const size_t weight_byte_offset =
                        weight.strides[3] * ofm +
                        weight.strides[2] * ifm +
                        weight.strides[1] * w_y +
                        weight.strides[0] * w_x;

                    const int_fast32_t i_val = loadValueAsRawInt(fmt, in_b_ptr + input_byte_offset);
                    const int_fast32_t w_val = loadValueAsRawInt(fmt, (char *)weight_ptr + weight_byte_offset);

                    // This is ok since all of them fit into int32_t
                    sum = applyWrapRoundingToAccum(fmt, i_val * w_val, wrap, to_ne) + sum;
                }
            }
            sum = wrapOrSat(fmt, sum, wrap);
        }

        // The step here could be added to the loops instead of recalcing
        // if, but does the compiler fail to hoist them out???
        const size_t output_byte_offset =
            output.strides[2] * ofm +
            output.strides[1] * y +
            output.strides[0] * x;
        storeRawIntValue(fmt, sum, out_b_ptr + output_byte_offset);
    }
}

void FullyConnectedKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        const void * weight_ptr, tensor_desc_t weight,
        const void * bias_ptr, tensor_desc_t bias,
        bool wrap,  // true for WRAP, else SATURATE
        bool to_ne, // true for ROUND_TO_NE, else ROUND_TO_ZERO
        void * output_ptr, tensor_desc_t output)
{
    assert (fmt == TENSOR_C_FMT_Q78 || fmt == TENSOR_C_FMT_U8 || fmt == TENSOR_C_FMT_S8);

    const size_t batch_dim_num = output.dim_num - 1;
    assert (batch_dim_num >= 0 && batch_dim_num <= 3);

    const size_t core_dim_num = input.dim_num - batch_dim_num;
    assert ((core_dim_num == 1 && weight.dim_num == 2) ||
            (core_dim_num == 3 && (weight.dim_num == 2 || weight.dim_num == 4)));

    assert (bias.dim_num == !!bias_ptr);
    const bool bias_present = !!bias.dim_num;

    if (core_dim_num == 1)
    {
        assert (weight.dims[0] == input.dims[0]);
    }
    else if (weight.dim_num == 2)
    {
        assert (weight.dims[0] == input.dims[0] * input.dims[1] * input.dims[2]);
    }
    else
    {
        assert (weight.dims[0] == input.dims[0]);
        assert (weight.dims[1] == input.dims[1]);
        assert (weight.dims[2] == input.dims[2]);
    }

    assert (weight.dims[weight.dim_num - 1] == output.dims[0]);
    assert (!bias_present || bias.dims[0] == output.dims[0]);

    for (size_t i = 0; i < batch_dim_num; ++i)
    {
        assert (output.dims[i + 1] == input.dims[i + core_dim_num]);
    }

    assertStridesModSizeof(fmt, input);
    assertStridesModSizeof(fmt, weight);
    assertStridesModSizeof(fmt, bias);
    assertStridesModSizeof(fmt, output);

    const size_t tmp_batch_dims[3] =
    {
        (batch_dim_num > 0 ? output.dims[1] : 1),
        (batch_dim_num > 1 ? output.dims[2] : 1),
        (batch_dim_num > 2 ? output.dims[3] : 1),
    };

    const size_t tmp_input_dims[3] =
    {
        (core_dim_num == 3 ? input.dims[0] : 1),
        (core_dim_num == 3 ? input.dims[1] : 1),
        input.dims[core_dim_num - 1],
    };

    const size_t ofm_num = output.dims[0];

    for (size_t b2 = 0; b2 < tmp_batch_dims[2]; ++b2)
    for (size_t b1 = 0; b1 < tmp_batch_dims[1]; ++b1)
    for (size_t b0 = 0; b0 < tmp_batch_dims[0]; ++b0)
    for (size_t ofm = 0; ofm < ofm_num; ++ofm)
    {
        int_fast32_t sum =
            bias_present ? loadValueAsRawInt(fmt, (char *)bias_ptr + bias.strides[0] * ofm) : 0;

        for (size_t ifm = 0; ifm < tmp_input_dims[2]; ++ifm)
        for (size_t y = 0; y < tmp_input_dims[1]; ++y)
        for (size_t x = 0; x < tmp_input_dims[0]; ++x)
        {
            size_t weight_byte_offset = weight.strides[weight.dim_num-1] * ofm;
            if (core_dim_num == 1)
            {
                weight_byte_offset += weight.strides[0] * ifm;
            }
            else if (weight.dim_num == 2)
            {
                const size_t count = x + tmp_input_dims[0] * (y + tmp_input_dims[1] * ifm);
                weight_byte_offset += weight.strides[0] * count;
            }
            else
            {
                weight_byte_offset +=
                    weight.strides[2] * ifm +
                    weight.strides[1] * y +
                    weight.strides[0] * x;
            }

            const size_t input_byte_offset =
                (batch_dim_num > 2 ? input.strides[core_dim_num + 2] * b2 : 0) +
                (batch_dim_num > 1 ? input.strides[core_dim_num + 1] * b1 : 0) +
                (batch_dim_num > 0 ? input.strides[core_dim_num + 0] * b0 : 0) +
                input.strides[core_dim_num - 1] * ifm +
                (core_dim_num == 3 ? input.strides[1] * y : 0) +
                (core_dim_num == 3 ? input.strides[0] * x : 0);

            const int_fast32_t w_val = loadValueAsRawInt(fmt, (char *)weight_ptr + weight_byte_offset);
            const int_fast32_t i_val = loadValueAsRawInt(fmt, (char *)input_ptr + input_byte_offset);

            // This is ok since all of them fit into int32_t
            sum = applyWrapRoundingToAccum(fmt, i_val * w_val, wrap, to_ne) + sum;
        }

        sum = wrapOrSat(fmt, sum, wrap);

        const size_t output_byte_offset =
            (batch_dim_num > 2 ? output.strides[3] * b2 : 0) +
            (batch_dim_num > 1 ? output.strides[2] * b1 : 0) +
            (batch_dim_num > 0 ? output.strides[1] * b0 : 0) +
            output.strides[0] * ofm;

        storeRawIntValue(fmt, sum, (char *)output_ptr + output_byte_offset);
    }
}

void PoolingKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        bool max_pooling,   // MAX vs AVG pooling
        size_t size_x, size_t size_y,
        size_t pad_x, size_t pad_y,
        size_t stride_x, size_t stride_y,
        void * output_ptr, tensor_desc_t output)
{
    assert(input.dim_num == 3 || input.dim_num == 4);
    assert(output.dim_num == input.dim_num);

    const size_t input_w = input.dims[0];
    const size_t input_h = input.dims[1];
    const size_t input_c = input.dims[2];
    const size_t input_b = input.dim_num > 3 ? input.dims[3] : 1;

    const size_t output_w = output.dims[0];
    const size_t output_h = output.dims[1];
    const size_t output_c = output.dims[2];
    const size_t output_b = output.dim_num > 3 ? output.dims[3] : 1;

    assert(input_w + 2 * pad_x >= size_x);
    assert(input_h + 2 * pad_y >= size_y);
//    assert(missing_div_with_round_mode((input_w + 2 * pad_x - size_x), stride_x) + 1 == output_w);
//    assert(missing_div_with_round_mode((input_h + 2 * pad_y - size_y), stride_y) + 1 == output_h);

    //TODO: verify this is enforced by the input/output validators
    assert(output_c == input_c);
    assert(output_b == input_b);

    // Since we calc offsets manually and cast to (int16_t *), we expect the-
    // alignment to be correct already
    assertStridesModSizeof (fmt, input);
    assertStridesModSizeof (fmt, output);

    //TODO: previously there was a 1d/3d stride for ofm but there's no 1D pool, right?

    // Input and output pointers for the current batch being processed,
    // Note: The compiler should've been able to hoist this out... And
    // there's a bunch of other possible hoising iopportunities here.
    const char * in_b_ptr = input_ptr;
    char * out_b_ptr = output_ptr;

    for (size_t b = 0; b < output_b; ++b, in_b_ptr += input.strides[3], out_b_ptr += output.strides[3])
    for (size_t c = 0; c < output_c; ++c)
    for (size_t y = 0; y < output_h; ++y)
    for (size_t x = 0; x < output_w; ++x)
    {
        int32_t result = max_pooling ? getMinValue(fmt) : 0;

        const size_t xx_start = CLAMP(x * stride_x,          pad_x, input_w + pad_x) - pad_x;
        const size_t xx_after = CLAMP(x * stride_x + size_x, pad_x, input_w + pad_x) - pad_x;

        const size_t yy_start = CLAMP(y * stride_y,          pad_y, input_h + pad_y) - pad_y;
        const size_t yy_after = CLAMP(y * stride_y + size_y, pad_y, input_h + pad_y) - pad_y;

        for (size_t yy = yy_start; yy < yy_after; ++yy)
        for (size_t xx = xx_start; xx < xx_after; ++xx)
        {
            const size_t input_byte_offset =
                input.strides[2] * c +
                input.strides[1] * yy +
                input.strides[0] * xx;
            const int32_t i_val = loadValueAsRawInt(fmt, in_b_ptr + input_byte_offset);

            result = max_pooling? MAX(result, i_val) : (result + i_val);
        }

        if (!max_pooling)
        {
            //result = conversion_24_8(result / (int16_t)(size_x * size_y));
            result = CLAMP(result / (size_x * size_y), getMinValue(fmt), getMaxValue(fmt));
        }

        const size_t output_byte_offset =
            output.strides[2] * c +
            output.strides[1] * y +
            output.strides[0] * x;
        storeRawIntValue(fmt, result, out_b_ptr + output_byte_offset);
    }
}

void SoftmaxKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        void * output_ptr, tensor_desc_t output)
{
//TODO: @Tomer, should we allow extra batch dims beyond 4? conv and poll have upto 3 of them! if not we can just discard this define and its usage
#define SOFTMAX_ALLOW_EXTRA_DIMS
    
#ifdef SOFTMAX_ALLOW_EXTRA_DIMS
    assert(input.dim_num >= 1 && input.dim_num <= 4);
#else
    assert(input.dim_num >= 1 && input.dim_num < MAX_NUM_OF_DIMENSIONS);
#endif

    assert(input.dim_num == output.dim_num);

    // Since we calc offsets manually and cast to (int16_t *), we expect the-
    // alignment to be correct already
    assertStridesModSizeof (fmt, input);
    assertStridesModSizeof (fmt, output);

    // We precalc and store the key (summation) index and the rest of the dims
    // which describe batching before the main loop for clarity, since the later may be partially shifted depending on the key dim.

    size_t key_sz = 0;
    size_t key_in_stride = 0;

#ifdef SOFTMAX_ALLOW_EXTRA_DIMS
    size_t batch_sz[5] = { 1, 1, 1, 1, 1 };
    size_t batch_in_strides[5] = { 0 };
    size_t batch_out_strides[5] = { 0 };
#else
    size_t batch_sz[3] = { 1, 1, 1 };
    size_t batch_in_strides[3] = { 0 };
    size_t batch_out_strides[3] = { 0 };
#endif

#if 1
    {
        size_t key = input.dim_num > 2 ? 2 : 0;

        key_sz = input.dims[key];
        key_in_stride = input.strides[key];
        
        for (size_t i = 0; i < input.dim_num - 1; ++i)
        {
            size_t idx = i < key ? i : i + 1;

            batch_sz[i] = input.dims[idx];
            batch_in_strides[i] = input.strides[idx];
            batch_out_strides[i] = output.strides[idx];
        }
    }
#else
    switch (input.dim_num)
    {
#ifdef SOFTMAX_ALLOW_EXTRA_DIMS
    case 6:
        batch_sz[4] = input.dims[5];
        batch_in_strides[4] = input.strides[5];
        batch_out_strides[4] = output.strides[5];
        /* fallthrough */
    case 5:
        batch_sz[3] = input.dims[4];
        batch_in_strides[3] = input.strides[4];
        batch_out_strides[3] = output.strides[4];
        /* fallthrough */
#endif
    case 4:
        batch_sz[2] = input.dims[3];
        batch_in_strides[2] = input.strides[3];
        batch_out_strides[2] = output.strides[3];
        /* fallthrough */
    case 3:
        key_sz = input.dims[2];
        key_in_stride = input.strides[2];

        batch_sz[1] = input.dims[1];
        batch_in_strides[1] = input.strides[1];
        batch_out_strides[1] = output.strides[1];

        batch_sz[0] = input.dims[0];
        batch_in_strides[0] = input.strides[0];
        batch_out_strides[0] = output.strides[0];
        break;
    case 2:
        batch_sz[0] = input.dims[1];
        batch_in_strides[0] = input.strides[1];
        batch_out_strides[0] = output.strides[1];
        /* fallthrough */
    case 1:
        key_sz = input.dims[0];
        key_in_stride = input.strides[0];
        break;
    default:
        assert(0);
    }
#endif

// The main loop calculation can be done with a double accumulator, float with
// value normalization (exp(val-max_val)) to avoid getting to inf or plain -
// float. Leaving all 3 here for result comparision, since the spec has nothing
// about required accumulator width.
//
// Note: For U8, S8 all 3 will result in the same results. But for Q78, because
//       summing exp(127) is quite large for a single precision float, using it
//       may already result in inf within the summation causing all values to
//       0 after softmax! And obviously for F32, the change of getting there is
//       even higher.
//
// Set to 0 for float, 1 for double, 2 for float with norm.
#define SOFTMAX_ACCUM_TYPE 0

#ifdef SOFTMAX_ALLOW_EXTRA_DIMS
    for (size_t b4 = 0; b4 < batch_sz[4]; ++b4)
    for (size_t b3 = 0; b3 < batch_sz[3]; ++b3)
#endif
    for (size_t b2 = 0; b2 < batch_sz[2]; ++b2)
    for (size_t b1 = 0; b1 < batch_sz[1]; ++b1)
    for (size_t b0 = 0; b0 < batch_sz[0]; ++b0)
    {
        // Input and output pointers for the current batch being processed.
        const char * in_b_ptr = (char*)input_ptr +
            batch_in_strides[2] * b2 +
            batch_in_strides[1] * b1 +
            batch_in_strides[0] * b0;
        char * out_b_ptr = (char*)output_ptr +
            batch_out_strides[2] * b2 +
            batch_out_strides[1] * b1 +
            batch_out_strides[0] * b0;

#ifdef SOFTMAX_ALLOW_EXTRA_DIMS
            in_b_ptr += batch_in_strides[4] * b4 + batch_in_strides[3] * b3;
            out_b_ptr += batch_out_strides[4] * b4 + batch_out_strides[3] * b3;
#endif

#if SOFTMAX_ACCUM_TYPE == 0
        float sum = 0.f;

        for (size_t i = 0; i < key_sz; ++i)
        {
            const int_fast32_t in = loadValueAsRawInt(fmt, in_b_ptr + key_in_stride * i);
            float in_val = unquantize(fmt, in);

            sum += expf(in_val);
        }

        for (size_t i = 0; i < key_sz; ++i)
        {
            const int_fast32_t in = loadValueAsRawInt(fmt, in_b_ptr + key_in_stride * i);
            float in_val = unquantize(fmt, in);

            storeRawIntValue(fmt, quantize(fmt, expf(in_val) / sum), out_b_ptr + key_in_stride * i);
        }
#elif SOFTMAX_ACCUM_TYPE == 1
        double sum = 0.;

        for (size_t i = 0; i < key_sz; ++i)
        {
            const int16_t * in_ptr = (int16_t *)(in_b_ptr + key_in_stride * i);
            float in_val = UNQUANTIZE(*in_ptr);

            sum += exp(in_val);
        }

        for (size_t i = 0; i < key_sz; ++i)
        {
            const int16_t * in_ptr = (int16_t *)(in_b_ptr + key_in_stride * i);
            float in_val = UNQUANTIZE(*in_ptr);

            int16_t * out_ptr = (int16_t *)(out_b_ptr + key_in_stride * i);
            *out_ptr = QUANTIZE(exp(in_val) / sum);
        }
#elif SOFTMAX_ACCUM_TYPE == 2
        float max_val = -FLT_MAX;
        float sum = 0.f;

        for (size_t i = 0; i < key_sz; ++i)
        {
            const int16_t * in_ptr = (int16_t *)(in_b_ptr + key_in_stride * i);
            float in_val = UNQUANTIZE(*in_ptr);

            max_val = MAX(max_val, in_val);
        }
        
        // Note: It may be benificial to cache the exponents
        for (size_t i = 0; i < key_sz; ++i)
        {
            const int16_t * in_ptr = (int16_t *)(in_b_ptr + key_in_stride * i);
            float in_val = UNQUANTIZE(*in_ptr);

            sum += expf(in_val - max_val);
        }

        for (size_t i = 0; i < key_sz; ++i)
        {
            const int16_t * in_ptr = (int16_t *)(in_b_ptr + key_in_stride * i);
            float in_val = UNQUANTIZE(*in_ptr);

            int16_t * out_ptr = (int16_t *)(out_b_ptr + key_in_stride * i);
            *out_ptr = QUANTIZE(expf(in_val - max_val) / sum);
        }
#else
#error SOFTMAX_ACCUM_TYPE must be 0..2
#endif
    }
}



/****************************************************************************
 *                                                                          *
 *                          SampleROIPoolingKernel                          *
 *                                                                          *
 ***************************************************************************/

void ROIPoolingKernelImpl(
        enum TensorCFmt fmt,
        const void * in0_ptr, tensor_desc_t in0,
        const void * in1_ptr, tensor_desc_t in1,
        void * out_ptr, tensor_desc_t out)
{
    assert ((in0.dim_num == 3 && in1.dim_num == 2 && out.dim_num == 4) ||
            (in0.dim_num == 4 && in1.dim_num == 3 && out.dim_num == 5));

    // format: [batch][channels][height][width]
    const size_t data_w = in0.dims[0];
    const size_t data_h = in0.dims[1];
    const size_t data_c = in0.dims[2];
    const size_t data_b = in0.dim_num == 4 ? in0.dims[3]: 1;

    // format: [batch][roi_count][4]
    const size_t rois_d = in1.dims[0];
    const size_t rois_r = in1.dims[1];
    const size_t rois_b = in1.dim_num == 3 ? in1.dims[2] : 1;

    // format: [batch][roi_count][channels][height][width]
    const size_t out_w = out.dims[0];
    const size_t out_h = out.dims[1];
    const size_t out_c = out.dims[2];
    const size_t out_r = out.dims[3];
    const size_t out_b = out.dim_num == 5 ? out.dims[4] : 1;

    assert(data_c == out_c);
    assert(data_b == rois_b && data_b == out_b);
    assert(rois_d == 4);
    assert(rois_r == out_r);

    //TODO: missing assert on strides % base type size
    //TODO: add comment about templating over the format outside of the loops

    const int lowest_val = getMinValue(fmt);

    for (size_t b = 0; b < out_b; ++b)
    for (size_t r = 0; r < out_r; ++r)
    for (size_t c = 0; c < out_c; ++c)
    for (size_t y = 0; y < out_h; ++y)
    for (size_t x = 0; x < out_w; ++x)
    {
        const char * roi_b_ptr = (char*)in1_ptr + in1.strides[1] * r + in1.strides[2] * b;

        const int roi_x0 = loadValueAsRawInt(fmt, roi_b_ptr + in1.strides[0] * 0);
        const int roi_y0 = loadValueAsRawInt(fmt, roi_b_ptr + in1.strides[0] * 1);
        const int roi_x1 = loadValueAsRawInt(fmt, roi_b_ptr + in1.strides[0] * 2);
        const int roi_y1 = loadValueAsRawInt(fmt, roi_b_ptr + in1.strides[0] * 3);

        // The final coordinate is within the ROI => +1
        // And we treat malformed dimensions as 1
        const int roi_w = MAX(roi_x1 - roi_x0, 0) + 1;
        const int roi_h = MAX(roi_y1 - roi_y0, 0) + 1;

        // Note that "after" is rounded up else we get the last cell,
        // instead of the cell beyond.
        //
        // For ex. with src being a 6 cell row and dst being a 4 cell one:
        // >>> [((x + 0) * 6) // 4 for x in range(4)]   # "begin" values
        // [0, 1, 3, 4]                                 # as expected
        // >>> [((x + 1) * 6) // 4 for x in range(4)]   # "after" values
        // [1, 3, 4, 6]                                 # [2, 3, 5, 6] expected!
        const int dx_begin = ((x + 0) * roi_w) / out_w;
        const int dy_begin = ((y + 0) * roi_h) / out_h;
        const int dx_after = ((x + 1) * roi_w + (out_w - 1)) / out_w;
        const int dy_after = ((y + 1) * roi_h + (out_h - 1)) / out_h;

        // clamp in case roi_x or roi_y were unreasonable
        const int x_begin = CLAMP(roi_x0 + dx_begin, 0, data_w);
        const int y_begin = CLAMP(roi_y0 + dy_begin, 0, data_h);
        const int x_after = CLAMP(roi_x0 + dx_after, 0, data_w);
        const int y_after = CLAMP(roi_y0 + dy_after, 0, data_h);

        const char * data_b_ptr = (char*)in0_ptr + in0.strides[3] * b + in0.strides[2] * c;

        // If there's no values for the current roi, we default to 0
        const bool non_empty = (x_begin < x_after && y_begin < y_after);
        int res = non_empty ? lowest_val : 0;

        for (int yy = y_begin; yy < y_after; ++yy)
        for (int xx = x_begin; xx < x_after; ++xx)
        {
            const void * val_ptr = data_b_ptr + in0.strides[1] * yy + in0.strides[0] * xx;
            int val = loadValueAsRawInt(fmt, val_ptr);

            res = MAX(res, val);
        }

        const size_t output_byte_offset =
            out.strides[4] * b +
            out.strides[3] * r + out.strides[2] * c +
            out.strides[1] * y + out.strides[0] * x;
        storeRawIntValue(fmt, res, (char*)out_ptr + output_byte_offset);
    }
}


/****************************************************************************
 *                                                                          *
 *                          SampleDeconvolutionKernel                       *
 *                                                                          *
 ***************************************************************************/

void DeconvolutionKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        const void * weight_ptr, tensor_desc_t weight,
        const void * bias_ptr, tensor_desc_t bias,
        vx_size pad_x, vx_size pad_y,
        vx_size upscale_x, vx_size upscale_y,
        bool wrap,  // true for WRAP, else SATURATE
        bool to_ne, // true for ROUND_TO_NE, else ROUND_TO_ZERO
        vx_size a_x, vx_size a_y,
        void * output_ptr, tensor_desc_t output)
{
    assert(input.dim_num == 3 || input.dim_num == 4);
    assert(weight.dim_num == 4);
    assert(bias.dim_num == 0 || bias.dim_num == 1 || bias.dim_num == 3);
    assert(output.dim_num == input.dim_num);

    const size_t input_w = input.dims[0];
    const size_t input_h = input.dims[1];
    const size_t input_c = input.dims[2];
    const size_t input_b = input.dim_num > 3 ? input.dims[3] : 1;

    const size_t weight_w = weight.dims[0];
    const size_t weight_h = weight.dims[1];
    const size_t weight_ifm = weight.dims[2];
    const size_t weight_ofm = weight.dims[3];

    const bool bias_present = !!bias.dim_num;
    const bool bias_shared = bias.dim_num == 1;
    const size_t bias_w = bias.dim_num > 0 ? bias.dims[0] : 0;
    const size_t bias_h = bias.dim_num > 1 ? bias.dims[1] : 1;
    const size_t bias_ofm = bias.dim_num > 2 ? bias.dims[2] : 1;

    const size_t output_w = output.dims[0];
    const size_t output_h = output.dims[1];
    const size_t output_c = output.dims[2];
    const size_t output_b = output.dim_num > 3 ? output.dims[3] : 1;

    assert(weight_ifm == input_c);
    assert(weight_ofm == output_c);

    assert(upscale_x > 0 && upscale_y > 0);
    assert(a_x < upscale_x && a_y < upscale_y);
    assert((input_w - 1) * upscale_x + weight_w + a_x > 2 * pad_x);
    assert((input_h - 1) * upscale_y + weight_h + a_y > 2 * pad_y);
    assert(output_w == (input_w - 1) * upscale_x + weight_w + a_x - 2 * pad_x);
    assert(output_h == (input_h - 1) * upscale_y + weight_h + a_y - 2 * pad_y);

    assert(weight_w >= pad_x + 1);
    assert(weight_h >= pad_y + 1);
    const size_t start_x_pad = weight_w - pad_x - 1;
    const size_t start_y_pad = weight_h - pad_y - 1;

    // NOTE:
    // The complete input width being sampled is,
    //  start_x_pad + ((input_w - 1) * upscale_x + 1) + after_x_pad
    // which is
    //  (input_w - 1) * upscale_x + 2 * weight_w - 2 * pad_x - 1 + a_x
    // and the stride being 1, the output width comes down to this plus 1 - weight_w
    // which ends up being
    //  (input_w - 1) * upscale_x + weight_w - 2 * pad_x + a_x
    // which is exactly output_w

    if (bias_shared)
    {
        assert(bias_w == weight_ofm);
    }
    else if (bias_present)
    {
        assert(bias_w == output_w);
        assert(bias_h == output_h);
        assert(bias_ofm == output_c);
    }

    assert(output_b == input_b);

    assertStridesModSizeof(fmt, input);
    assertStridesModSizeof(fmt, weight);
    assertStridesModSizeof(fmt, bias);
    assertStridesModSizeof(fmt, output);

    // Input and output pointers for the current batch being processed,
    // Note: The compiler should've been able to hoist this out... And
    // there's a bunch of other possible hoising iopportunities here.
    const char * in_b_ptr = input_ptr;
    char * out_b_ptr = output_ptr;

    for (size_t b = 0; b < output_b; ++b)
    for (size_t ofm = 0; ofm < output_c; ++ofm)
    for (size_t y = 0; y < output_h; ++y)
    for (size_t x = 0; x < output_w; ++x)
    {
        int32_t sum = 0;
        if (bias_present)
        {
            const size_t bias_byte_offset =
                bias_shared
                ? (bias.strides[0] * ofm)
                : (bias.strides[2] * ofm + bias.strides[1] * y + bias.strides[0] * x);

            sum = loadValueAsRawInt(fmt, (char *)bias_ptr + bias_byte_offset);
        }
        
        for (size_t ifm = 0; ifm < input_c; ++ifm)
        {
            for (size_t w_y = 0; w_y < weight_h; ++w_y)
            for (size_t w_x = 0; w_x < weight_w; ++w_x)
            {
                if (x + w_x >= start_x_pad && x + w_x < input_w + start_x_pad &&
                    y + w_y >= start_y_pad && y + w_y < input_h + start_y_pad)
                {
                    const size_t xx = x + w_x - start_x_pad;
                    const size_t yy = y + w_y - start_y_pad;

                    if (xx % upscale_x == 0 && yy % upscale_y == 0)
                    {
                        const size_t input_byte_offset =
                            (b ? input.strides[3] * b : 0) +
                            input.strides[2] * ifm +
                            input.strides[1] * (yy / upscale_y) +
                            input.strides[0] * (xx / upscale_x);
                        const size_t weight_byte_offset =
                            weight.strides[3] * ofm +
                            weight.strides[2] * ifm +
                            weight.strides[1] * w_y +
                            weight.strides[0] * w_x;

                        const int_fast32_t i_val = loadValueAsRawInt(fmt, in_b_ptr + input_byte_offset);
                        const int_fast32_t w_val = loadValueAsRawInt(fmt, (char *)weight_ptr + weight_byte_offset);

                        // This is ok since all of them fit into int32_t
                        sum = applyWrapRoundingToAccum(fmt, i_val * w_val, wrap, to_ne) + sum;
                    }
                }
            }
            sum = wrapOrSat(fmt, sum, wrap);
        }

        // The step here could be added to the loops instead of recalcing
        // if, but does the compiler fail to hoist them out???
        const size_t output_byte_offset =
            (b ? output.strides[3] * b : 0) +
            output.strides[2] * ofm +
            output.strides[1] * y +
            output.strides[0] * x;
        storeRawIntValue(fmt, sum, out_b_ptr + output_byte_offset);
    }
}

typedef struct _pwl_table
{
    int32_t quantization;
    int16_t *lut;
} pwl_table;

static vx_int16 mul_truncate(int32_t input, int32_t trunc_bits)
{
    int32_t temp_res = input >> trunc_bits;
    int32_t truncatedMSB = (input >> (trunc_bits-1)) & 0x1;
    temp_res += truncatedMSB;

    int16_t upperBound = (1 << (15)) - 1;
    int16_t lowerBound = -(1 << (15));
    if (temp_res >= upperBound)
    {
        return upperBound;
    }
    else if (temp_res <= lowerBound)
    {
        return lowerBound;
    }
    return (vx_int16)temp_res;
}

static vx_int16 quantize_q78(double x, int bits)
{
    double x_scaled = x * pow(2,bits);

    vx_int16 maxVal = (1 << (16 - 1)) - 1; // if numBits == 8, the number should be 127, allowing range of [-127,+127]
    vx_int16 minVal = -(1 << (16 - 1));

    if (x_scaled > maxVal)
    {
        return maxVal;
    }

    if (x_scaled < minVal)
    {
        return minVal;
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
static vx_int16 pwl3(vx_int32 val, pwl_table *pwl_table, vx_bool symmetric)
{
    int64_t tmp;
    vx_int16 *table = pwl_table->lut;
    vx_bool negative = vx_false_e;
    if ((val & 0x80000000) != 0)
    {
        negative = vx_true_e;
    }
    if (symmetric && negative)
    {
        val = -val;
    }

    // Check range and round if needed
    int relevantIndex = 0;
    if (val <= table[0])
    {
        val = table[0];
        relevantIndex = 0;
    }
    else if (val >= table[1])
    {
        val = table[1];
        relevantIndex = PWL_NORM_NUM_SEGMENTS - 1;
    }
    else
    {
        for (int i = 0; i < PWL_NORM_NUM_SEGMENTS; i++)
        {
            if ((val >= table[2 + i]) && (val < table[2 + i + 1]))
            {
                relevantIndex = i;
                break;
            }
        }
    }
    // Compute approximated PWL:
    // Input, segments and output are Q1.7.8. A,B are quantized differently.
    tmp = (val * table[3 + PWL_NORM_NUM_SEGMENTS + relevantIndex]);
    tmp += (table[3 + (2 * PWL_NORM_NUM_SEGMENTS) + relevantIndex] * 256);
    tmp = mul_truncate((int32_t)tmp, pwl_table->quantization);
    if (symmetric && negative)
    {
        tmp = -tmp;
    }

    return (vx_int16)tmp;
}


// table[0-1] = myRanges, table[2-34] = mySegments, table[35-66] = myA, table[67-98] = myB
static pwl_table *createPwlNormLut(vx_float32 beta)
{
    vx_int16 table_size = 3 * (PWL_NORM_NUM_SEGMENTS + 1);
    vx_int16* table = (vx_int16*)malloc(table_size * sizeof(vx_int16));
    vx_float32* tmp_table = (vx_float32*)malloc(table_size * sizeof(vx_float32));
    if (table == NULL || tmp_table == NULL)
    {
        return NULL;
    }

    //Ranges
    tmp_table[0] = 0;
    tmp_table[1] = 128.0f;

    //Segments
    vx_float32 *S = tmp_table + 2;
    for (int i = 0; i <= PWL_NORM_NUM_SEGMENTS; i++)
        S[i] = (tmp_table[0] + i*(tmp_table[1] - tmp_table[0]) / PWL_NORM_NUM_SEGMENTS);

    // Compute the a,b values for each segment
    vx_float32 *A = tmp_table + 3 + PWL_NORM_NUM_SEGMENTS;
    vx_float32 *B = tmp_table + 3 + 2 * PWL_NORM_NUM_SEGMENTS;
    for (int i = 0; i < PWL_NORM_NUM_SEGMENTS; i++) {
        float x1 = S[i], x2 = S[i + 1];
        float y1 = pow(1.0f+x1, (-1) * beta), y2 = pow(1.0f + x2, (-1) * beta);
        A[i] = ((y2 - y1) / (x2 - x1)); // Simple equations of line between 2 points
        B[i] = ((y2*x1 - y1*x2) / (x1 - x2));
    }

    // Segments should be Q1.7.8 since this is the input format
    int i = 0;
    for (; i <= 2+PWL_NORM_NUM_SEGMENTS; i++)
    {
        table[i] = quantize_q78(tmp_table[i], 8);
    }

    // Q1.1.14 - gives good results, since all A and B values are between -2 and 2
    for (; i < table_size; i++) {
        table[i] = quantize_q78(tmp_table[i], 14);
    }

    free(tmp_table);

    pwl_table *result = (pwl_table *) malloc(sizeof(pwl_table));
    if (result == NULL)
    {
        return NULL;
    }
    result->lut = table;
    result->quantization = 14;
    return result;
}




static void destroyPwlLutN(pwl_table *table)
{
    if (table != NULL)
    {
        if (table->lut != NULL)
        {
            free(table->lut);
        }

        free(table);
    }
}



vx_status NNNormalizationKernelImpl(vx_tensor inputs, vx_scalar type_scalar, vx_scalar norm_size_scalar, vx_scalar alpha_scalar, vx_scalar beta_scalar, vx_tensor outputs)
{
    vx_int16 *pinput, *poutput;
    vx_int32 min_ifm, max_ifm, num_ifms, min_allowed_ifm, max_allowed_ifm;
    vx_size num_dims_in,num_dims_out, batch_size, iw, ih;
    vx_size offset;
    vx_status status = VX_SUCCESS;
    vx_enum norm_type;
    vx_size norm_size;
    vx_float32 alpha_f, beta_f;
    //vx_int16 alpha, beta, k = QUANTIZE(1.0);
    vx_size input_dimensions[MAX_NUM_OF_DIMENSIONS] = {0}, input_stride[MAX_NUM_OF_DIMENSIONS] = {0};
    vx_size output_dimensions[MAX_NUM_OF_DIMENSIONS] = {0}, output_stride[MAX_NUM_OF_DIMENSIONS] = {0};
    status |= AllocatePatch (inputs, &num_dims_in, input_dimensions, input_stride, (void**)&pinput, VX_READ_ONLY);
    status |= AllocatePatch (outputs, &num_dims_out, output_dimensions, output_stride,(void**)&poutput, VX_WRITE_ONLY);

    status |= vxCopyScalar(type_scalar, &norm_type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(norm_size_scalar, &norm_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(alpha_scalar, &alpha_f, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(beta_scalar, &beta_f, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    //alpha = float2short(alpha_f);
    //beta = float2short(beta_f);

    batch_size = num_dims_in > 3 ? (input_dimensions[3]) : 1;


    num_ifms = input_dimensions[2];
    ih = input_dimensions[1];
    iw = input_dimensions[0];

    min_allowed_ifm = 0;
    max_allowed_ifm = num_ifms;

    // Host code - create the PWC LUT
    pwl_table *table = createPwlNormLut(beta_f);
    if (table == NULL)
    {
        return VX_ERROR_NO_MEMORY;
    }
    vx_int16 scaling_factor;

    // Normalizing
    if (norm_type == VX_NN_NORMALIZATION_ACROSS_MAPS)
    {
        scaling_factor = quantize_q78(sqrt(alpha_f/(vx_uint32)norm_size), 8);
        for (vx_uint32 batch_iter = 0; batch_iter < batch_size; batch_iter++)
        {
            for (vx_int32 ifm = 0; ifm < (vx_int32)num_ifms; ifm++)
            {
                max_ifm = ((ifm + (vx_int32)norm_size / 2) >(max_allowed_ifm - 1) ? (max_allowed_ifm - 1) : (ifm + norm_size / 2));
                min_ifm = ((ifm - (vx_int32)norm_size / 2 < min_allowed_ifm) ? min_allowed_ifm : (ifm - norm_size / 2));
                for (vx_int32 y = 0; y < (int)ih; y++)
                {
                    for (vx_int32 x = 0; x < (int)iw; x++)
                    {
                        // Scaling factor is expected to be a small number by definition, so 32 bits are more than enough
                        vx_int32 pwl_in = 0;
                        for (vx_int32 curr_ifm = min_ifm; curr_ifm <= max_ifm; curr_ifm++)
                        {
                            offset = (batch_iter * (vx_int32)input_stride[3] + curr_ifm * (vx_int32)input_stride[2] + y * (vx_int32)input_stride[1] + x * (vx_int32)input_stride[0]) / 2; //TODO: Assuming 16 bits elements
                            vx_int16 scaled_input = mul_truncate((vx_int32)pinput[offset] * scaling_factor, 8);
                            vx_int32 scaled_input_sq = (vx_int32)scaled_input * scaled_input;
                            pwl_in += scaled_input_sq;
                        }

                        offset = (batch_iter * input_stride[3] + ifm * input_stride[2] + y * input_stride[1] + x * input_stride[0]) / 2; //TODO: Assuming 16 bits elements
                        vx_int16 tmp2 = pwl3(mul_truncate(pwl_in, 8), table, vx_false_e);
                        poutput[offset] = mul_truncate((vx_int32)pinput[offset] * tmp2, 8);
                       // printf ("offset %d pwl_in %d tmp2 %d output %d\n", offset, pwl_in, (int)tmp2, (int)poutput[offset]);
                    }
                }
            }
        }
    }
    else // VX_NN_NORMALIZATION_SAME_MAP
    {
        scaling_factor = quantize_q78(sqrt(alpha_f)/norm_size, 8);
        for (vx_uint32 batch_iter = 0; batch_iter < batch_size; batch_iter++)
        {
            for (vx_int32 ifm = 0; ifm < num_ifms; ifm++)
            {
                for (vx_int32 y = 0; y < (int)ih; y++)
                {
                    for (vx_int32 x = 0; x < (int)iw; x++)
                    {
                        vx_int32 pwl_in = 0;
                        for (vx_uint32 ny = 0; ny < norm_size; ny++)
                        {
                            for (vx_uint32 nx = 0; nx < norm_size; nx++)
                            {
                                vx_int32 currXind = x - (norm_size / 2) + nx;
                                vx_int32 currYind = y - (norm_size / 2) + ny;
                                if ((currYind >= 0) && (currXind >= 0) && (currYind < (int)ih) && (currXind < (int)iw))
                                {
                                    offset = (batch_iter * input_stride[3] + ifm*input_stride[2] +
                                        currYind*input_stride[1] + currXind * input_stride[0]) / 2; //TODO: Assuming 16 bits elements
                                    vx_int32 scaled_input = mul_truncate((vx_int32)pinput[offset] * scaling_factor, 8);
                                    vx_int32 scaled_input_sq = (vx_int32)scaled_input * scaled_input;
                                    pwl_in += scaled_input_sq;
                                }
                            }
                        }

                        offset = (batch_iter * input_stride[3] + ifm * input_stride[2] + y * input_stride[1] + x * input_stride[0]) / 2; //TODO: Assuming 16 bits elements
                        vx_int16 tmp2 = pwl3(mul_truncate(pwl_in, 8), table, vx_false_e);
                        poutput[offset] = mul_truncate((vx_int32)pinput[offset] * tmp2, 8);
                    }
                }
            }
        }
    }

    // Destroy PWC LUT
    destroyPwlLutN(table);

    status |= ReleasePatch (inputs, num_dims_in, input_dimensions, input_stride, (void**)&pinput, VX_READ_ONLY);
    status |= ReleasePatch (outputs, num_dims_out, output_dimensions, output_stride, (void**)&poutput, VX_WRITE_ONLY);

    return status;
}


void ActivationKernelImpl(
        enum TensorCFmt fmt,
        const void * input_ptr, tensor_desc_t input,
        vx_enum func,   // MAX vs AVG pooling
        float a, float b,
        void * output_ptr, tensor_desc_t output)
{
    (void)a;
    (void)b;
    assert(input.dim_num > 0 || input.dim_num <= 4);
    assert(output.dim_num == input.dim_num);

    const size_t input_w = input.dims[0];
    const size_t input_h = input.dim_num > 1 ? input.dims[1] : 1;
    const size_t input_c = input.dim_num > 2 ? input.dims[2] : 1;
    const size_t input_b = input.dim_num > 3 ? input.dims[3] : 1;

    const size_t output_w = output.dims[0];
    const size_t output_h = output.dim_num > 1 ? output.dims[1] : 1;
    const size_t output_c = output.dim_num > 2 ? output.dims[2] : 1;
    const size_t output_b = output.dim_num > 3 ? output.dims[3] : 1;


    //TODO: verify this is enforced by the input/output validators
    assert(output_c == input_c);
    assert(output_b == input_b);
    assert(output_w == input_w);
    assert(output_h == input_h);

    // Since we calc offsets manually and cast to (int16_t *), we expect the-
    // alignment to be correct already
    assertStridesModSizeof (fmt, input);
    assertStridesModSizeof (fmt, output);


    //TODO: previously there was a 1d/3d stride for ofm but there's no 1D pool, right?

    // Input and output pointers for the current batch being processed,
    // Note: The compiler should've been able to hoist this out... And
    // there's a bunch of other possible hoising iopportunities here.
    const char * in_b_ptr = input_ptr;
    char * out_b_ptr = output_ptr;

    for (size_t b = 0; b < output_b; ++b, in_b_ptr += input.strides[3], out_b_ptr += output.strides[3])
    for (size_t c = 0; c < output_c; ++c)
    for (size_t y = 0; y < output_h; ++y)
    for (size_t x = 0; x < output_w; ++x)
    {
            const size_t input_byte_offset =
                input.strides[2] * c +
                input.strides[1] * y +
                input.strides[0] * x;
            const int32_t i_val = loadValueAsRawInt(fmt, in_b_ptr + input_byte_offset);
            int32_t result;
            switch (func)
            {/*
             case VX_NN_NONLINEAR_LOGISTIC:
             poutput[offset] = 1 / (1 + exp(0 - pinput[offset]));
             break;
             */
            case VX_NN_ACTIVATION_HYPERBOLIC_TAN:

                //result = pwl(i_val, lut);
                result = quantize(fmt, 2 / ( 1 + exp(-2*value2Float(fmt, i_val) )) - 1); // Use formula instead of LUT, not sure at all it works.

                break;
                /* relu and brelu are currently redundant as only unsigned int 16 is supported */
            case VX_NN_ACTIVATION_RELU:
                result = (i_val > 0 ? i_val : 0);
                break;
            case VX_NN_ACTIVATION_LOGISTIC:
                result = quantize(fmt, 1 / ( 1 + exp(-value2Float(fmt, i_val) ))); // TODO: only for q78, and i am not sure it is good!
                break;
            default:
                result = 0;
                break;
            }

            const size_t output_byte_offset =
                output.strides[2] * c +
                output.strides[1] * y +
                output.strides[0] * x;
            storeRawIntValue(fmt, result, out_b_ptr + output_byte_offset);
    }

}

#endif
