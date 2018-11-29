#include <c_model.h>
//#include "tensor_utils.h"

#include <assert.h>


#define Q78_FIXED_POINT_POSITION 8
#define Q78_SCALE   (1 << Q78_FIXED_POINT_POSITION)
#define Q78_HALF    (1 << (Q78_FIXED_POINT_POSITION - 1))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define CLAMP(val, lower, upper) MAX((lower), MIN((val), (upper)))

static C_KERNEL_INLINE int loadFormatted(const void* ptr, vx_enum type)
{
    switch (type)
    {
    case VX_TYPE_INT8: return *(vx_int8*)ptr;
    case VX_TYPE_UINT8: return *(vx_uint8*)ptr;
    case VX_TYPE_INT16: return *(vx_int16*)ptr;
    default: assert(0); return 0;
    }
}

static C_KERNEL_INLINE void storeSatFormatted(int val, const void* ptr, vx_enum type)
{
    switch (type)
    {
    case VX_TYPE_INT8: *(vx_int8*)ptr = CLAMP(val, INT8_MIN, INT8_MAX); break;
    case VX_TYPE_UINT8: *(vx_uint8*)ptr = CLAMP(val, 0, UINT8_MAX); break;
    case VX_TYPE_INT16: *(vx_int16*)ptr = CLAMP(val / Q78_SCALE, INT16_MIN, INT16_MAX); break;
    default: assert(0);
    }
}

static C_KERNEL_INLINE int addToMulAccumFormatted (int sum, int val, vx_enum type)
{
    switch (type)
    {
    case VX_TYPE_INT8: return sum + val;
    case VX_TYPE_UINT8: return sum + val;
    case VX_TYPE_INT16: return sum + val * Q78_SCALE;
    default: assert(0); return 0;
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

    for (size_t i = 0; i < dims1[1]; i++)
    for (size_t j = 0; j < dims2[0]; j++)
    {
        int sum = 0;
        for (size_t k = 0; k < dims1[0]; ++k)
        {
            const void * src1_ptr = (char*)src1 + src1_strides[1] * i + src1_strides[0] * k;
            const void * src2_ptr = (char*)src2 + src2_strides[1] * k + src2_strides[0] * j;

            int src1_val = loadFormatted(src1_ptr, type);
            int src2_val = loadFormatted(src2_ptr, type);

            sum += src1_val * src2_val;
        }

        if (src3)
        {
            const void * src3_ptr = (char*)src3 + src3_strides[1] * i + src3_strides[0] * j;
            int src3_val = loadFormatted(src3_ptr, type);
            sum = addToMulAccumFormatted(sum, src3_val, type);
        }

        const void * dst_ptr = (char*)dst + dst_strides[1] * i + dst_strides[0] * j;
        storeSatFormatted(sum, dst_ptr, type);
    }
}

