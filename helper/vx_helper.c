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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <VX/vx_lib_extras.h>
/* TODO: remove vx_compatibility.h after transition period */
#include <VX/vx_compatibility.h>

/*! \file vx_helper.c
 * \brief The OpenVX Helper Implementation.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \example vx_helper.c
 */


// no clue if this belongs here, but i don't know where better to put it. needed by c_scale.c and vx_scale.c
uint32_t math_gcd(uint32_t a, uint32_t b)
{
    uint32_t gcd = (a < b ? a : b);
    while (gcd > 0) {
        if ((a % gcd) == 0 && (b % gcd) == 0)
            return gcd;
        gcd -= 1;
    }
    return 0;
}


static vx_log_t helper_log;

static void vxInitLog(vx_log_t *log)
{
    log->first = -1;
    log->last = 0;
    log->count = VX_MAX_LOG_NUM_ENTRIES;
}

vx_status vxGetLogEntry(vx_reference r, char message[VX_MAX_LOG_MESSAGE_LEN])
{
    vx_status status = VX_SUCCESS;
    vx_int32  cur = 0;
    vx_bool isContext = (vxGetContext(r) == 0 ? vx_true_e : vx_false_e);

    // if there's nothing in the helper_log return success
    if (helper_log.first == -1)
    {
        return VX_SUCCESS;
    }

    // first, match the reference to the parameter r
    // if active mark not active and copy and return.
    // if not active move on.

    for (cur = helper_log.first; cur != helper_log.last; cur = (cur + 1)%helper_log.count)
    {
        // if reference match or context was given
        if (((isContext == vx_true_e) || (r == helper_log.entries[cur].reference)) &&
            (helper_log.entries[cur].active == vx_true_e))
        {
            status = helper_log.entries[cur].status;
            strncpy(message, helper_log.entries[cur].message, VX_MAX_LOG_MESSAGE_LEN);
            helper_log.entries[cur].active = vx_false_e;
            if (cur == helper_log.first)
            {
                //printf("Aged out first entry!\n");
                helper_log.first = (helper_log.first + 1)%helper_log.count;
                if (helper_log.first == helper_log.last)
                {
                    helper_log.first = -1;
                    //printf("Log is now empty!\n");
                }
            }
            break;
        }
    }
    return status;
}

static void VX_CALLBACK vxHelperLogCallback(vx_context context,
                                vx_reference ref,
                                vx_status status,
                                const vx_char string[])
{
    (void)context;
    helper_log.entries[helper_log.last].reference = ref;
    helper_log.entries[helper_log.last].status = status;
    helper_log.entries[helper_log.last].active = vx_true_e;
    strncpy(helper_log.entries[helper_log.last].message, string, VX_MAX_LOG_MESSAGE_LEN - 1);

    if (helper_log.first == -1)
        helper_log.first = helper_log.last;
    else if (helper_log.first == helper_log.last)
        helper_log.first = (helper_log.first + 1)%helper_log.count;
    helper_log.last = (helper_log.last + 1)%helper_log.count;
}

void vxRegisterHelperAsLogReader(vx_context context)
{
    vxInitLog(&helper_log);
    vxRegisterLogCallback(context, &vxHelperLogCallback, vx_false_e);
}

vx_bool vxFindOverlapRectangle(vx_rectangle_t* rect_a, vx_rectangle_t* rect_b, vx_rectangle_t* rect_res)
{
    vx_bool res = vx_false_e;

    if ((rect_a) && (rect_b) && (rect_res))
    {
        enum { sx = 0, sy = 1, ex = 2, ey = 3 };
        vx_uint32 a[4] = { rect_a->start_x, rect_a->start_y, rect_a->end_x, rect_a->end_y };
        vx_uint32 b[4] = { rect_b->start_x, rect_b->start_y, rect_b->end_x, rect_b->end_y };
        vx_uint32 c[4] = {0};

        c[sx] = (a[sx] > b[sx] ? a[sx] : b[sx]);
        c[sy] = (a[sy] > b[sy] ? a[sy] : b[sy]);
        c[ex] = (a[ex] < b[ex] ? a[ex] : b[ex]);
        c[ey] = (a[ey] < b[ey] ? a[ey] : b[ey]);

        if (c[sx] < c[ex] && c[sy] < c[ey])
        {
            rect_res->start_x = c[sx];
            rect_res->start_y = c[sy];
            rect_res->end_x   = c[ex];
            rect_res->end_y   = c[ey];

            res = vx_true_e;
        }
    }

    return res;
}

/* Note: with the current sample implementation structure, we have no other choice than
returning 0 in case of errors not due to vxCreateGenericNode, because vxGetErrorObject
is internal to another library and is not exported. This is not an issue since vxGetStatus
correctly manages a ref == 0 */
vx_node vxCreateNodeByStructure(vx_graph graph,
                                vx_enum kernelenum,
                                vx_reference params[],
                                vx_uint32 num)
{
    vx_status status = VX_SUCCESS;
    vx_node node = 0;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_kernel kernel = vxGetKernelByEnum(context, kernelenum);
    if (kernel)
    {
        node = vxCreateGenericNode(graph, kernel);
        if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
        {
            vx_uint32 p = 0;
            for (p = 0; p < num; p++)
            {
                status = vxSetParameterByIndex(node, p, params[p]);
                if (status != VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)graph, status, "Kernel %d Parameter %u is invalid.\n", kernelenum, p);
                    vxReleaseNode(&node);
                    node = 0;
                    break;
                }
            }
        }
        else
        {
            vxAddLogEntry((vx_reference)graph, VX_ERROR_INVALID_PARAMETERS, "Failed to create node with kernel enum %d\n", kernelenum);
            status = VX_ERROR_NO_MEMORY;
        }
        vxReleaseKernel(&kernel);
    }
    else
    {
        vxAddLogEntry((vx_reference)graph, VX_ERROR_INVALID_PARAMETERS, "failed to retrieve kernel enum %d\n", kernelenum);
        status = VX_ERROR_NOT_SUPPORTED;
    }
    return node;
}


void vxClearLog(vx_reference ref)
{
    char message[VX_MAX_LOG_MESSAGE_LEN];
    vx_status status = VX_SUCCESS;
    do {
        status = vxGetLogEntry(ref, message);
    } while (status != VX_SUCCESS);
}

vx_status vxLinkParametersByIndex(vx_node node_a, vx_uint32 index_a, vx_node node_b, vx_uint32 index_b) {
    vx_parameter param_a = vxGetParameterByIndex(node_a, index_a);
    vx_parameter param_b = vxGetParameterByIndex(node_b, index_b);
    vx_status status = vxLinkParametersByReference(param_a, param_b);
    vxReleaseParameter(&param_a);
    vxReleaseParameter(&param_b);
    return status;
}

vx_status vxLinkParametersByReference(vx_parameter a, vx_parameter b) {
    vx_status status = VX_SUCCESS;
    vx_enum dirs[2] = {0,0};
    vx_enum types[2] = {0,0};
    vx_reference ref = 0;

    status = vxQueryParameter(a, VX_PARAMETER_DIRECTION, &dirs[0], sizeof(dirs[0]));
    if (status != VX_SUCCESS)
        return status;

    status = vxQueryParameter(b, VX_PARAMETER_DIRECTION, &dirs[1], sizeof(dirs[1]));
    if (status != VX_SUCCESS)
        return status;

    status = vxQueryParameter(a, VX_PARAMETER_TYPE, &types[0], sizeof(types[0]));
    if (status != VX_SUCCESS)
        return status;

    status = vxQueryParameter(b, VX_PARAMETER_TYPE, &types[1], sizeof(types[1]));
    if (status != VX_SUCCESS)
        return status;

    if (types[0] == types[1])
    {
        if ((dirs[0] == VX_OUTPUT || dirs[0] == VX_BIDIRECTIONAL) && dirs[1] == VX_INPUT)
        {
            status = vxQueryParameter(a, VX_PARAMETER_REF, &ref, sizeof(ref));
            if (status != VX_SUCCESS)
                return status;
            status = vxSetParameterByReference(b, ref);
        }
        else if ((dirs[1] == VX_OUTPUT || dirs[1] == VX_BIDIRECTIONAL) && dirs[0] == VX_INPUT)
        {
            status = vxQueryParameter(b, VX_PARAMETER_REF, &ref, sizeof(ref));
            if (status != VX_SUCCESS)
                return status;
            status = vxSetParameterByReference(a, ref);
        }
        else
            status = VX_ERROR_INVALID_LINK;
    }
    return status;
}


vx_status vxSetAffineRotationMatrix(vx_matrix matrix,
                                    vx_float32 angle,
                                    vx_float32 scale,
                                    vx_float32 center_x,
                                    vx_float32 center_y)
{
    vx_status status = VX_FAILURE;
    vx_float32 mat[3][2];
    vx_size columns = 0ul, rows = 0ul;
    vx_enum type = 0;
    vxQueryMatrix(matrix, VX_MATRIX_COLUMNS, &columns, sizeof(columns));
    vxQueryMatrix(matrix, VX_MATRIX_ROWS, &rows, sizeof(rows));
    vxQueryMatrix(matrix, VX_MATRIX_TYPE, &type, sizeof(type));
    if ((columns == 2) && (rows == 3) && (type == VX_TYPE_FLOAT32))
    {
        status = vxCopyMatrix(matrix, mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        if (status == VX_SUCCESS)
        {
            vx_float32 radians = (angle / 360.0f) * (vx_float32)VX_TAU;
            vx_float32 a = scale * (vx_float32)cos(radians);
            vx_float32 b = scale * (vx_float32)sin(radians);
            mat[0][0] = a;
            mat[1][0] = b;
            mat[2][0] = ((1.0f - a) * center_x) - (b * center_y);
            mat[0][1] = -b;
            mat[1][1] = a;
            mat[2][1] = (b * center_x) + ((1.0f - a) * center_y);
            status = vxCopyMatrix(matrix, mat, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
        }
    }
    else
    {
        vxAddLogEntry((vx_reference)matrix, status, "Failed to set affine matrix due to type or dimension mismatch!\n");
    }
    return status;
}

vx_status vxAlterRectangle(vx_rectangle_t *rect,
                           vx_int32 dsx,
                           vx_int32 dsy,
                           vx_int32 dex,
                           vx_int32 dey)
{
    if (rect)
    {
        rect->start_x += dsx;
        rect->start_y += dsy;
        rect->end_x += dex;
        rect->end_y += dey;
        return VX_SUCCESS;
    }
    return VX_ERROR_INVALID_REFERENCE;
}

vx_status vxAddParameterToGraphByIndex(vx_graph g, vx_node n, vx_uint32 index)
{
    vx_parameter p = vxGetParameterByIndex(n, index);
    vx_status status = vxAddParameterToGraph(g, p);
    vxReleaseParameter(&p);
    return status;
}

void vxReadRectangle(const void *base,
                     const vx_imagepatch_addressing_t *addr,
                     const vx_border_t *borders,
                     vx_df_image type,
                     vx_uint32 center_x,
                     vx_uint32 center_y,
                     vx_uint32 radius_x,
                     vx_uint32 radius_y,
                     void *destination,
                     vx_uint32 border_x_start)
{
#if 0
    /* strides are not modified by scaled planes, they are byte distances from allocators */
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_uint32 center_offset = (center_y * stride_y) + (center_x * stride_x);
    vx_uint32 ky, kx;
    // kx, kx - kernel x and y
    for (ky = 0; ky <= 2 * radius_y; ++ky)
    {
        for (kx = 0; kx <= 2 * radius_x; ++kx)
        {
            vx_uint32 ckx = kx, cky = ky; // kernel x and y, clamped to image bounds
            vx_size dest_index = 0ul;

            if (kx < radius_x && center_x < radius_x - kx)
                ckx = radius_x - center_x;
            else if (kx > radius_x && addr->dim_x - center_x <= kx - radius_x)
                ckx = radius_x + (addr->dim_x - center_x - 1);

            if (ky < radius_y && center_y < radius_y - ky)
                cky = radius_y - center_y;
            else if (ky > radius_y && addr->dim_y - center_y <= ky - radius_y)
                cky = radius_y + (addr->dim_y - center_y - 1);

            dest_index = ky * (2 * radius_x + 1) + kx;

            if (borders->mode == VX_BORDER_MODE_CONSTANT && (kx != ckx || ky != cky))
            {
                switch (type) {
                case VX_DF_IMAGE_U8:
                    ((vx_uint8*)destination)[dest_index] = borders->constant_value; break;
                case VX_DF_IMAGE_U16:
                case VX_DF_IMAGE_S16:
                    ((vx_uint16*)destination)[dest_index] = borders->constant_value; break;
                case VX_DF_IMAGE_F32:
                    ((vx_float32*)destination)[dest_index] = borders->constant_value; break;
                default:
                    abort(); // add other types as needed
                }
            }
            else
            {
                vx_uint32 offset = center_offset
                        + (stride_y * cky) - (radius_y * stride_y)
                        + (stride_x * ckx) - (radius_x * stride_x);
                switch (type) {
                case VX_DF_IMAGE_U8:
                    ((vx_uint8*)destination)[dest_index] = *(vx_uint8*)(ptr + offset); break;
                case VX_DF_IMAGE_U16:
                case VX_DF_IMAGE_S16:
                    ((vx_uint16*)destination)[dest_index] = *(vx_uint16*)(ptr + offset); break;
                case VX_DF_IMAGE_F32:
                    ((vx_float32*)destination)[dest_index] = *(vx_float32*)(ptr + offset); break;
                default:
                    abort(); // add other types as needed
                }
            }
        }
    }
#else
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    vx_uint16 stride_x_bits = addr->stride_x_bits;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 dest_index = 0;
    // kx, ky - kernel x and y
    if( borders->mode == VX_BORDER_REPLICATE || borders->mode == VX_BORDER_UNDEFINED )
    {
        for (ky = -(int32_t)radius_y; ky <= (int32_t)radius_y; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            y = y < 0 ? 0 : y >= height ? height - 1 : y;

            for (kx = -(int32_t)radius_x; kx <= (int32_t)radius_x; ++kx, ++dest_index)
            {
                vx_int32 x = (int32_t)(center_x + kx);
                x = x < (int32_t)border_x_start ? (int32_t)border_x_start : x >= width ? width - 1 : x;

                switch(type)
                {
                case VX_DF_IMAGE_U1:
                    ((vx_uint8*)destination)[dest_index] =
                        ( *(vx_uint8*)(ptr + y*stride_y + vxDivFloor(x*stride_x_bits, 8)) & (1 << (x % 8)) ) >> (x % 8);
                    break;
                case VX_DF_IMAGE_U8:
                    ((vx_uint8*)destination)[dest_index] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
                    break;
                case VX_DF_IMAGE_S16:
                case VX_DF_IMAGE_U16:
                    ((vx_uint16*)destination)[dest_index] = *(vx_uint16*)(ptr + y*stride_y + x*stride_x);
                    break;
                case VX_DF_IMAGE_S32:
                case VX_DF_IMAGE_U32:
                    ((vx_uint32*)destination)[dest_index] = *(vx_uint32*)(ptr + y*stride_y + x*stride_x);
                    break;
                case VX_DF_IMAGE_F32:
                    ((vx_float32*)destination)[dest_index] = *(vx_float32*)(ptr + y*stride_y + x*stride_x);
                    break;
                default:
                    abort();
                }
            }
        }
    }
    else if( borders->mode == VX_BORDER_CONSTANT )
    {
        vx_pixel_value_t cval = borders->constant_value;
        for (ky = -(int32_t)radius_y; ky <= (int32_t)radius_y; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            int ccase_y = y < 0 || y >= height;

            for (kx = -(int32_t)radius_x; kx <= (int32_t)radius_x; ++kx, ++dest_index)
            {
                vx_int32 x = (int32_t)(center_x + kx);
                int ccase = ccase_y || x < (int32_t)border_x_start || x >= width;

                switch(type)
                {
                    case VX_DF_IMAGE_U1:
                        if( !ccase )
                            ((vx_uint8*)destination)[dest_index] =
                                ( *(vx_uint8*)(ptr + y*stride_y + vxDivFloor(x*stride_x_bits, 8)) & (1 << (x % 8)) ) >> (x % 8);
                        else
                            ((vx_uint8*)destination)[dest_index] = (vx_uint8)(cval.U1 ? 1 : 0);
                        break;
                    case VX_DF_IMAGE_U8:
                        if( !ccase )
                            ((vx_uint8*)destination)[dest_index] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
                        else
                            ((vx_uint8*)destination)[dest_index] = (vx_uint8)cval.U8;
                        break;
                    case VX_DF_IMAGE_S16:
                    case VX_DF_IMAGE_U16:
                        if( !ccase )
                            ((vx_uint16*)destination)[dest_index] = *(vx_uint16*)(ptr + y*stride_y + x*stride_x);
                        else
                            ((vx_uint16*)destination)[dest_index] = (vx_uint16)cval.U16;
                        break;
                    case VX_DF_IMAGE_S32:
                    case VX_DF_IMAGE_U32:
                        if( !ccase )
                            ((vx_uint32*)destination)[dest_index] = *(vx_uint32*)(ptr + y*stride_y + x*stride_x);
                        else
                            ((vx_uint32*)destination)[dest_index] = (vx_uint32)cval.U32;
                        break;
                    case VX_DF_IMAGE_F32:
                        if( !ccase )
                            ((vx_float32*)destination)[dest_index] = *(vx_float32*)(ptr + y*stride_y + x*stride_x);
                        else
                        {
                            const vx_uint8* p = (const vx_uint8*)&cval.reserved;
                            ((vx_float32*)destination)[dest_index] = *(vx_float32*)p;
                        }
                        break;
                    default:
                        abort();
                }
            }
        }
    }
    else
        abort();
#endif
}

// Integer division with rounding towards minus infinity
vx_int64 vxDivFloor(vx_int64 x, vx_int64 y) {
    vx_int64 q = x / y;
    vx_int64 r = x % y;
    if ( (r != 0) && ((r < 0) != (y < 0)) )
        --q;
    return q;
}

#if __STDC_VERSION__ == 199901L // C99

static vx_float32 vxh_matrix_trace_f32(vx_size columns, vx_size rows, vx_float32 matrix[rows][columns]) {
    vx_float32 trace = 0.0f;
    vx_size i = 0ul;
    for (i = 0ul; i < columns && i < rows; i++) {
        trace += matrix[i][i];
    }
    return trace;
}


static vx_int32 vxh_matrix_trace_i32(vx_size columns, vx_size rows, vx_int32 matrix[rows][columns]) {
    vx_int32 trace = 0;
    vx_size i = 0ul;
    for (i = 0ul; i < columns && i < rows; i++) {
        trace += matrix[i][i];
    }
    return trace;
}

vx_status vxMatrixTrace(vx_matrix matrix, vx_scalar trace) {
    vx_size columns = 0u;
    vx_size rows = 0u;
    vx_status status = VX_SUCCESS;
    vx_enum mtype = VX_TYPE_INVALID, stype = VX_TYPE_INVALID;

    status |= vxQueryMatrix(matrix, VX_MATRIX_COLUMNS, &columns, sizeof(columns));
    status |= vxQueryMatrix(matrix, VX_MATRIX_ROWS, &rows, sizeof(rows));
    status |= vxQueryMatrix(matrix, VX_MATRIX_TYPE, &mtype, sizeof(mtype));
    status |= vxQueryScalar(trace, VX_SCALAR_TYPE, &stype, sizeof(stype));

    if (status != VX_SUCCESS)
        return VX_ERROR_INVALID_REFERENCE;
    if (mtype == VX_TYPE_INVALID || mtype != stype)
        return VX_ERROR_INVALID_TYPE;
    if (columns == 0 || columns != rows)
        return VX_ERROR_INVALID_DIMENSION;

    if (stype == VX_TYPE_INT32) {
        vx_int32 mat[rows][columns];
        vx_int32 t = 0;
        status |= vxCopyMatrix(matrix, mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        t = vxh_matrix_trace_i32(columns, rows, mat);
        status |= vxCopyScalar(trace, &t, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    } else if (stype == VX_TYPE_FLOAT32) {
        vx_float32 mat[rows][columns];
        vx_float32 t = 0.0f;
        status |= vxCopyMatrix(matrix, mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        t = vxh_matrix_trace_f32(columns, rows, mat);
        status |= vxCopyScalar(trace, &t, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    }
    return status;
}

static vx_float32 vxh_matrix_determinant_f32(vx_size columns, vx_size rows, vx_float32 matrix[rows][columns]) {
    vx_float32 det = 0.0f;
    if (rows == 2 && columns == 2) {
        det = (matrix[0][0] * matrix[1][1]) - (matrix[1][0] * matrix[0][1]);
    } else if (rows == 3 && columns == 3) {
        det = ((matrix[0][0]*matrix[1][1]*matrix[2][2]) +
               (matrix[0][1]*matrix[1][2]*matrix[2][0]) +
               (matrix[0][2]*matrix[1][0]*matrix[2][1]) -
               (matrix[0][2]*matrix[1][1]*matrix[2][0]) -
               (matrix[0][1]*matrix[1][0]*matrix[2][2]) -
               (matrix[0][0]*matrix[1][2]*matrix[2][1]));
    }
    return det;
}

static void vxh_matrix_inverse_f32(vx_uint32 columns, vx_uint32 rows, vx_float32 out[rows][columns], vx_float32 in[rows][columns]) {
    if (rows == 2 && columns == 2) {
        vx_float32 detinv = 1.0f / vxh_matrix_determinant_f32(columns,rows,in);
        out[0][0] = detinv *  in[1][1];
        out[0][1] = detinv * -in[0][1];
        out[1][0] = detinv * -in[1][0];
        out[1][1] = detinv *  in[0][0];
    }
    /*! \bug implement 3x3 */
}

vx_status vxMatrixInverse(vx_matrix input, vx_matrix output) {
    vx_size ci, co;
    vx_size ri, ro;
    vx_enum ti, to;
    vx_status status = VX_SUCCESS;
    status |= vxQueryMatrix(input, VX_MATRIX_COLUMNS, &ci, sizeof(ci));
    status |= vxQueryMatrix(input, VX_MATRIX_ROWS, &ri, sizeof(ri));
    status |= vxQueryMatrix(input, VX_MATRIX_TYPE, &ti, sizeof(ti));
    status |= vxQueryMatrix(output, VX_MATRIX_COLUMNS, &co, sizeof(co));
    status |= vxQueryMatrix(output, VX_MATRIX_ROWS, &ro, sizeof(ro));
    status |= vxQueryMatrix(output, VX_MATRIX_TYPE, &to, sizeof(to));
    if (status != VX_SUCCESS)
        return status;
    if (ci != co || ri != ro || ci != ri)
        return VX_ERROR_INVALID_DIMENSION;
    if (ti != to)
        return VX_ERROR_INVALID_TYPE;
    if (ti == VX_TYPE_FLOAT32) {
        vx_float32 in[ri][ci];
        vx_float32 ou[ro][co];
        vxCopyMatrix(input, in, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        vxh_matrix_inverse_f32(ci, ri, ou, in);
        vxCopyMatrix(output, ou, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    }
    /*! \bug implement integer */
    return status;
}

#endif

