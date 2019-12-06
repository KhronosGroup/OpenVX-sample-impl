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

/*!
 * \file
 * \brief The Warp Affine and Perspective Kernels
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include "vx_internal.h"
#include <c_model.h>

static vx_status VX_CALLBACK vxWarpPerspectiveKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    if (num == 4)
    {
        vx_image  src_image = (vx_image) parameters[0];
        vx_matrix matrix    = (vx_matrix)parameters[1];
        vx_scalar stype     = (vx_scalar)parameters[2];
        vx_image  dst_image = (vx_image) parameters[3];

        vx_border_t borders;
        vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

        return vxWarpPerspective(src_image, matrix, stype, dst_image, &borders);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxWarpAffineKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    if (num == 4)
    {
        vx_image  src_image = (vx_image) parameters[0];
        vx_matrix matrix    = (vx_matrix)parameters[1];
        vx_scalar stype     = (vx_scalar)parameters[2];
        vx_image  dst_image = (vx_image) parameters[3];

        vx_border_t borders;
        vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

        return vxWarpAffine(src_image, matrix, stype, dst_image, &borders);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status vxWarpInputValidator(vx_node node, vx_uint32 index, vx_size mat_columns)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if ((format == VX_DF_IMAGE_U8) ||
                (format == VX_DF_IMAGE_U1 && mat_columns == 2))
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_matrix matrix;
            vxQueryParameter(param, VX_PARAMETER_REF, &matrix, sizeof(matrix));
            if (matrix)
            {
                vx_enum data_type = 0;
                vx_size rows = 0ul, columns = 0ul;
                vxQueryMatrix(matrix, VX_MATRIX_TYPE, &data_type, sizeof(data_type));
                vxQueryMatrix(matrix, VX_MATRIX_ROWS, &rows, sizeof(rows));
                vxQueryMatrix(matrix, VX_MATRIX_COLUMNS, &columns, sizeof(columns));
                if ((data_type == VX_TYPE_FLOAT32) && (columns == mat_columns) && (rows == 3))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseMatrix(&matrix);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vx_enum interp = 0;
                    vxCopyScalar(scalar, &interp, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((interp == VX_INTERPOLATION_NEAREST_NEIGHBOR) ||
                        (interp == VX_INTERPOLATION_BILINEAR))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxWarpAffineInputValidator(vx_node node, vx_uint32 index)
{
    return vxWarpInputValidator(node, index, 2);
}

static vx_status VX_CALLBACK vxWarpPerspectiveInputValidator(vx_node node, vx_uint32 index)
{
    return vxWarpInputValidator(node, index, 3);
}

static vx_status VX_CALLBACK vxWarpOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter mtx_param = vxGetParameterByIndex(node, 1);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if ((vxGetStatus((vx_reference)src_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)mtx_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS))
        {
            vx_matrix mtx = 0;
            vx_image src = 0, dst = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            vxQueryParameter(mtx_param, VX_PARAMETER_REF, &mtx, sizeof(mtx));
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            if (src && mtx && dst)
            {
                vx_size mtx_cols = 0ul;
                vx_uint32 w1 = 0, h1 = 0;
                vx_df_image f0 = VX_DF_IMAGE_VIRT, f1 = VX_DF_IMAGE_VIRT;

                vxQueryImage(dst, VX_IMAGE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(dst, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(src, VX_IMAGE_FORMAT, &f0, sizeof(f0));
                vxQueryImage(dst, VX_IMAGE_FORMAT, &f1, sizeof(f1));
                vxQueryMatrix(mtx, VX_MATRIX_COLUMNS, &mtx_cols, sizeof(mtx_cols));
                /* output can not be virtual */
                // TODO handle affive vx perspective
                if ( (w1 != 0) && (h1 != 0) && (f0 == f1) &&
                     (f1 == VX_DF_IMAGE_U8 || (f1 == VX_DF_IMAGE_U1 && mtx_cols == 2)) )
                {
                    /* fill in the meta data with the attributes so that the checker will pass */
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = f1;
                    ptr->dim.image.width = w1;
                    ptr->dim.image.height = h1;
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&src);
                vxReleaseImage(&dst);
                vxReleaseMatrix(&mtx);
            }
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&mtx_param);
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

static vx_param_description_t warp_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_MATRIX, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t warp_affine_kernel = {
    VX_KERNEL_WARP_AFFINE,
    "org.khronos.openvx.warp_affine",
    vxWarpAffineKernel,
    warp_kernel_params, dimof(warp_kernel_params),
    NULL,
    vxWarpAffineInputValidator,
    vxWarpOutputValidator,
    NULL,
    NULL,
};

vx_kernel_description_t warp_perspective_kernel = {
    VX_KERNEL_WARP_PERSPECTIVE,
    "org.khronos.openvx.warp_perspective",
    vxWarpPerspectiveKernel,
    warp_kernel_params, dimof(warp_kernel_params),
    NULL,
    vxWarpPerspectiveInputValidator,
    vxWarpOutputValidator,
    NULL,
    NULL,
};
