#include <c_model.h>
#include "conversion_utils.h"
#include "tensor_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <VX/vx_types.h>

#include <assert.h>
//#include <stdio.h> //TODO: remove me

vx_status TransposeTensorKernelImpl(
        vx_tensor in1,
        vx_scalar sc_dim1,
        vx_scalar sc_dim2,
        vx_tensor out,
        vx_size el_size)
{

    vx_status status = VX_SUCCESS;

    void* in1_data = NULL;
    void* out_data = NULL;


    vx_size dim1, dim2;
    //TODO: do we want to assert all 3 formats match here?

    vx_size in1_dim_num = 0, in1_dims[MAX_NUM_OF_DIMENSIONS] = {0}, in1_strides[MAX_NUM_OF_DIMENSIONS] = {0};
    vx_size out_dim_num = 0, out_dims[MAX_NUM_OF_DIMENSIONS] = {0}, out_strides[MAX_NUM_OF_DIMENSIONS] = {0};

    status |= AllocatePatch(in1, &in1_dim_num, in1_dims, in1_strides, &in1_data, VX_READ_ONLY);
    status |= AllocatePatch(out, &out_dim_num, out_dims, out_strides, &out_data, VX_WRITE_ONLY);

    status |= vxCopyScalar(sc_dim1, &dim1, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(sc_dim2, &dim2, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vx_size out_size = ComputeNumberOfElements(out_dims, out_dim_num);

    //TODO: check params values! overfloat (and rounding for MUL) are ignored atm!!!
    
    // This doesn't apply to a JIT/ parametrized AOT like OCL but,
    // The #if below compares the naive version with hoisting the conditions out of the loop.
    // Further splitting can be done for the no-broadcast case (equal dims) and especially for the
    // flat memory case which would make it possible to sub it with a simple simd array op.

    int tmp = (int)out_strides[dim1];
    out_strides[dim1] = out_strides[dim2];
    out_strides[dim2] = tmp;
    for (vx_size i = 0; i < out_size; ++i)
    {
        vx_size in1_pos0 = 0, out_pos0 = 0;
        vx_size in1_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, in1_strides, in1_dim_num, &in1_pos0);

        vx_size out_pos = ComputeGlobalPositionsFromIndex(i, in1_dims, out_strides, in1_dim_num, &out_pos0);

        memcpy ((vx_uint8*)out_data+out_pos, (vx_uint8*)in1_data+in1_pos, el_size);

    }
    tmp = (int)out_strides[dim1];
    out_strides[dim1] = out_strides[dim2];
    out_strides[dim2] = tmp;
    status |= ReleasePatch (in1, in1_dim_num, in1_dims, in1_strides, &in1_data, VX_READ_ONLY);
    status |= ReleasePatch (out, out_dim_num, out_dims, out_strides, &out_data, VX_WRITE_ONLY);

    return status;
}

