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
 * \brief The File IO Object Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_debug_ext Debugging Extension
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>
#include "debug_k.h"


/*! \brief Declares the parameter types for \ref vxFWriteImageNode.
* \ingroup group_debug_ext
*/
static
vx_param_description_t fwrite_image_kernel_params[] =
{
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownFWriteImageKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(fwrite_image_kernel_params))
    {
        vx_image input = (vx_image)parameters[0];
        vx_array file  = (vx_array)parameters[1];
        status = ownFWriteImage(input, file);
    }

    return status;
} /* ownFWriteImageKernel */

static
vx_status VX_CALLBACK own_fwrite_image_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(fwrite_image_kernel_params) && NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2))
        {
            vx_image image = 0;
            vx_array fname = 0;

            status = VX_SUCCESS;

            status |= vxQueryParameter(param1, VX_PARAMETER_REF, &image, sizeof(image));
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &fname, sizeof(fname));

            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)image) &&
                VX_SUCCESS == vxGetStatus((vx_reference)fname))
            {
                vx_df_image format = 0;

                status |= vxQueryImage(image, VX_IMAGE_FORMAT, &format, sizeof(format));

                /* validate input image */
                if (VX_SUCCESS == status &&
                    ((VX_DF_IMAGE_U8 == format) ||
                    (VX_DF_IMAGE_S16 == format) ||
                    (VX_DF_IMAGE_U16 == format) ||
                    (VX_DF_IMAGE_U32 == format) ||
                    (VX_DF_IMAGE_UYVY == format) ||
                    (VX_DF_IMAGE_YUYV == format) ||
                    (VX_DF_IMAGE_IYUV == format) ||
                    (VX_DF_IMAGE_RGB == format)))
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;

                /* validate input array */
                if (VX_SUCCESS == status)
                {
                    vx_map_id map_id = 0;
                    vx_char* filename = NULL;
                    vx_size filename_stride = 0;

                    status |= vxMapArrayRange(fname, 0, VX_MAX_FILE_NAME, &map_id, &filename_stride, (void**)&filename, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                    if (VX_SUCCESS == status && sizeof(vx_char) == filename_stride)
                    {
                        if (strncmp(filename, "", VX_MAX_FILE_NAME) == 0)
                        {
                            vxAddLogEntry((vx_reference)node, VX_FAILURE, "Empty file name. %s\n", filename);
                            status = VX_ERROR_INVALID_VALUE;
                        }
                        else
                        {
                            status = VX_SUCCESS;
                        }
                    }
                    else
                        status = VX_ERROR_INVALID_PARAMETERS;

                    vxUnmapArrayRange(fname, map_id);
                }
            }

            if (NULL != image)
                vxReleaseImage(&image);

            if (NULL != fname)
                vxReleaseArray(&fname);
        }
        else
            status = VX_ERROR_INVALID_PARAMETERS;

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);
    } /* if ptrs non NULL */

    return status;
} /* own_fwrite_image_validator() */


/*! \brief Declares the parameter types for \ref vxFWriteArrayNode.
* \ingroup group_debug_ext
*/
static
vx_param_description_t fwrite_array_kernel_params[] =
{
    { VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownFWriteArrayKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(fwrite_array_kernel_params))
    {
        vx_array arr  = (vx_array)parameters[0];
        vx_array file = (vx_array)parameters[1];

        vx_size num_items;
        vx_size item_size;
        vx_size stride;
        void* arrptr = NULL;
        vx_char* filename = NULL;
        vx_size filename_stride = 0;
        FILE* fp = NULL;
        vx_size i;
        vx_map_id fname_map_id = 0;
        vx_map_id arr_map_id = 0;

        status = vxMapArrayRange(file, 0, VX_MAX_FILE_NAME, &fname_map_id, &filename_stride, (void**)&filename, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        if (status != VX_SUCCESS || filename_stride != sizeof(vx_char))
        {
            vxUnmapArrayRange(file, fname_map_id);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
            return VX_FAILURE;
        }

        fp = fopen(filename, "wb+");
        if (fp == NULL)
        {
            vxUnmapArrayRange(file, fname_map_id);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n", filename);
            return VX_FAILURE;
        }

        status |= vxQueryArray(arr, VX_ARRAY_NUMITEMS, &num_items, sizeof(num_items));
        status |= vxQueryArray(arr, VX_ARRAY_ITEMSIZE, &item_size, sizeof(item_size));

        status |= vxMapArrayRange(arr, 0, num_items, &arr_map_id, &stride, &arrptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        if (status == VX_SUCCESS)
        {
            if (fwrite(&num_items, sizeof(num_items), 1, fp) == sizeof(num_items) &&
                fwrite(&item_size, sizeof(item_size), 1, fp) == sizeof(item_size))
            {
                for (i = 0; i < num_items; ++i)
                {
                    if (fwrite(vxFormatArrayPointer(arrptr, i, stride), item_size, 1, fp) != item_size)
                    {
                        status = VX_FAILURE;
                        break;
                    }
                }
            }
            else
            {
                status = VX_FAILURE;
            }

            vxUnmapArrayRange(arr, arr_map_id);
        }

        fclose(fp);
        vxUnmapArrayRange(file, fname_map_id);
    }

    return status;
} /* ownFWriteArrayKernel() */


/*! \brief Declares the parameter types for \ref vxFReadImageNode.
* \ingroup group_debug_ext
*/
static
vx_param_description_t fread_image_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownFReadImageKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(fread_image_kernel_params))
    {
        vx_array file   = (vx_array)parameters[0];
        vx_image output = (vx_image)parameters[1];

        status = ownFReadImage(file, output);
    }

    return status;
} /* ownFReadImageKernel() */

static
vx_status VX_CALLBACK own_fread_image_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(fwrite_image_kernel_params) && NULL != metas)
    {
        vx_parameter param = 0;

        param = vxGetParameterByIndex(node, 0);

        status = vxGetStatus((vx_reference)param);
        if (VX_SUCCESS == status)
        {
            vx_array fname = 0;

            status |= vxQueryParameter(param, VX_PARAMETER_REF, &fname, sizeof(fname));

            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)fname))
            {
                vx_map_id map_id = 0;
                vx_char* filename = NULL;
                vx_size filename_stride = 0;

                status |= vxMapArrayRange(fname, 0, VX_MAX_FILE_NAME, &map_id, &filename_stride, (void**)&filename, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                if (VX_SUCCESS == status && sizeof(vx_char) == filename_stride)
                {
                    if (strncmp(filename, "", VX_MAX_FILE_NAME) == 0)
                    {
                        vxAddLogEntry((vx_reference)node, VX_FAILURE, "Empty file name. %s\n", filename);
                        status = VX_ERROR_INVALID_VALUE;
                    }
                    else
                    {
                        vx_char* ext = NULL;
                        FILE* fp = NULL;
                        vx_char tmp[VX_MAX_FILE_NAME];
                        vx_uint32 width = 0;
                        vx_uint32 height = 0;
                        vx_df_image format = VX_DF_IMAGE_VIRT;

                        fp = fopen(filename, "rb");
                        if (fp != NULL)
                        {
                            ext = strrchr(filename, '.');
                            if (ext)
                            {
                                vx_uint32 depth = 0;
                                if ((strcmp(ext, ".pgm") == 0 || strcmp(ext, ".PGM") == 0))
                                {
                                    FGETS(tmp, fp); // PX
                                    FGETS(tmp, fp); // comment
                                    FGETS(tmp, fp); // W H
                                    sscanf(tmp, "%u %u", &width, &height);
                                    FGETS(tmp, fp); // BPP
                                    sscanf(tmp, "%u", &depth);
                                    if (UINT8_MAX == depth)
                                        format = VX_DF_IMAGE_U8;
                                    else if (INT16_MAX == depth)
                                        format = VX_DF_IMAGE_S16;
                                    else if (UINT16_MAX == depth)
                                        format = VX_DF_IMAGE_U16;
                                }
                                else if (strcmp(ext, ".bw") == 0)
                                {
                                    vx_char shortname[256] = { 0 };
                                    vx_char fmt[5] = { 0 };
                                    vx_int32 cbps = 0;
                                    sscanf(filename, "%256[a-zA-Z]_%ux%u_%4[A-Z0-9]_%db.bw", shortname, &width, &height, fmt, &cbps);
                                    if (strcmp(fmt, "P400") == 0)
                                    {
                                        format = VX_DF_IMAGE_U8;
                                    }
                                }
                                else if (strcmp(ext, ".yuv") == 0)
                                {
                                    vx_char shortname[256] = { 0 };
                                    vx_char fmt[5] = { 0 };
                                    vx_int32 cbps = 0;
                                    sscanf(filename, "%256[a-zA-Z]_%ux%u_%4[A-Z0-9]_%db.bw", shortname, &width, &height, fmt, &cbps);
                                    if (strcmp(fmt, "IYUV") == 0)
                                    {
                                        format = VX_DF_IMAGE_IYUV;
                                    }
                                    else if (strcmp(fmt, "UYVY") == 0)
                                    {
                                        format = VX_DF_IMAGE_UYVY;
                                    }
                                    else if (strcmp(fmt, "P444") == 0)
                                    {
                                        format = VX_DF_IMAGE_YUV4;
                                    }
                                    else if (strcmp(fmt, "YUY2") == 0)
                                    {
                                        format = VX_DF_IMAGE_YUYV;
                                    }
                                }
                                else if (strcmp(ext, ".rgb") == 0)
                                {
                                    vx_char shortname[256] = { 0 };
                                    vx_char fmt[5] = { 0 };
                                    vx_int32 cbps = 0;
                                    sscanf(filename, "%256[a-zA-Z]_%ux%u_%4[A-Z0-9]_%db.rgb", shortname, &width, &height, fmt, &cbps);
                                    if (strcmp(fmt, "I444") == 0)
                                    {
                                        format = VX_DF_IMAGE_RGB;
                                    }
                                } // if file extension is supported
                                else
                                    status = VX_ERROR_INVALID_PARAMETERS;
                            } // if file extension is non empty
                            else
                                status = VX_ERROR_INVALID_PARAMETERS;
                        }
                        else
                        {
                            vxAddLogEntry((vx_reference)node, VX_FAILURE, "Failed to open file %s\n", filename);
                            status = VX_FAILURE;
                        }

                        if (VX_SUCCESS == status)
                        {
                            vxSetMetaFormatAttribute(metas[1], VX_IMAGE_WIDTH,  &width,  sizeof(width));
                            vxSetMetaFormatAttribute(metas[1], VX_IMAGE_HEIGHT, &height, sizeof(height));
                            vxSetMetaFormatAttribute(metas[1], VX_IMAGE_FORMAT, &format, sizeof(format));
                        }

                        if (NULL != fp)
                            fclose(fp);

                        status = VX_SUCCESS;
                    } // if filename is non empty
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;

                vxUnmapArrayRange(fname, map_id);
            }
            else
                status = VX_ERROR_INVALID_PARAMETERS;

            if (NULL != fname)
                vxReleaseArray(&fname);
        }

        if (NULL != param)
            vxReleaseParameter(&param);
    } // if ptrs non NULL

    return status;
} /* own_fread_image_validator() */


/*! \brief Declares the parameter types for \ref vxFReadArrayNode.
* \ingroup group_debug_ext
*/
static
vx_param_description_t fread_array_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownFReadArrayKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (NULL != node && NULL != parameters && num == dimof(fread_array_kernel_params))
    {
        vx_array file = (vx_array)parameters[0];
        vx_array arr  = (vx_array)parameters[1];

        vx_size num_items;
        vx_size item_size;
        vx_size arr_capacity;
        vx_size arr_item_size;
        vx_char* filename = NULL;
        vx_size filename_stride = 0;
        FILE* fp = NULL;
        vx_map_id map_id = 0;

        status = vxMapArrayRange(file, 0, VX_MAX_FILE_NAME, &map_id, &filename_stride, (void**)&filename, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        if (status != VX_SUCCESS || filename_stride != sizeof(vx_char))
        {
            vxUnmapArrayRange(file, map_id);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
            return VX_FAILURE;
        }

        fp = fopen(filename, "wb+");
        if (fp == NULL)
        {
            vxUnmapArrayRange(file, map_id);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n", filename);
            return VX_FAILURE;
        }

        if (fread(&num_items, sizeof(num_items), 1, fp) == sizeof(num_items) &&
            fread(&item_size, sizeof(item_size), 1, fp) == sizeof(item_size))
        {
            vxQueryArray(arr, VX_ARRAY_CAPACITY, &arr_capacity, sizeof(arr_capacity));
            vxQueryArray(arr, VX_ARRAY_ITEMSIZE, &arr_item_size, sizeof(arr_item_size));

            if (arr_capacity >= num_items && arr_item_size == item_size)
            {
                void* tmpbuf = malloc(num_items * item_size);
                if (tmpbuf)
                {
                    if (fread(tmpbuf, item_size, num_items, fp) == (num_items * item_size))
                    {
                        status = VX_SUCCESS;
                        status |= vxTruncateArray(arr, 0);
                        status |= vxAddArrayItems(arr, num_items, tmpbuf, item_size);
                    }
                    free(tmpbuf);
                }
                else
                {
                    status = VX_ERROR_NO_MEMORY;
                }
            }
            else
            {
                vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect destination array "VX_FMT_REF"\n", arr);
                status = VX_FAILURE;
            }
        }
        else
        {
            status = VX_FAILURE;
        }

        fclose(fp);
        vxUnmapArrayRange(file, map_id);
    } // if ptrs non NULL

    return status;
}  /* ownFReadArrayKernel() */

static
vx_status VX_CALLBACK own_all_pass_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;
    (void)parameters;
    (void)num;
    (void)metas;

    return VX_SUCCESS;
} /* own_all_pass_validator() */


vx_kernel_description_t fwrite_image_kernel =
{
    VX_KERNEL_DEBUG_FWRITE_IMAGE,
    "org.khronos.debug.fwrite_image",
    ownFWriteImageKernel,
    fwrite_image_kernel_params, dimof(fwrite_image_kernel_params),
    own_fwrite_image_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};

vx_kernel_description_t fwrite_array_kernel =
{
    VX_KERNEL_DEBUG_FWRITE_ARRAY,
    "org.khronos.debug.fwrite_array",
    ownFWriteArrayKernel,
    fwrite_array_kernel_params, dimof(fwrite_array_kernel_params),
    own_all_pass_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};

vx_kernel_description_t fread_image_kernel =
{
    VX_KERNEL_DEBUG_FREAD_IMAGE,
    "org.khronos.debug.fread_image",
    ownFReadImageKernel,
    fread_image_kernel_params, dimof(fread_image_kernel_params),
    own_fread_image_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};

vx_kernel_description_t fread_array_kernel =
{
    VX_KERNEL_DEBUG_FREAD_ARRAY,
    "org.khronos.debug.fread_array",
    ownFReadArrayKernel,
    fread_array_kernel_params, dimof(fread_array_kernel_params),
    own_all_pass_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
