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

#include <stdio.h>
#include <string.h>
#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include "debug_k.h"

vx_status ownFWriteImage(vx_image input, vx_array file)
{
    FILE* fp = NULL;
    vx_char* ext = NULL;
    size_t wrote = 0ul;
    vx_char* filename = NULL;
    vx_size filename_stride = 0;
    vx_uint32 p;
    vx_uint32 y;
    vx_uint32 sx;
    vx_uint32 ex;
    vx_uint32 sy;
    vx_uint32 ey;
    vx_uint32 width;
    vx_uint32 height;
    vx_size planes;
    vx_uint8* src[4] = { NULL, NULL, NULL, NULL };
    vx_imagepatch_addressing_t addr[4] =
    {
        VX_IMAGEPATCH_ADDR_INIT,
        VX_IMAGEPATCH_ADDR_INIT,
        VX_IMAGEPATCH_ADDR_INIT,
        VX_IMAGEPATCH_ADDR_INIT
    };
    vx_df_image format;
    vx_rectangle_t rect;
    vx_map_id fname_map_id = 0;
    vx_map_id plane_map_id[4] = { 0, 0, 0, 0 };
    vx_status status = VX_SUCCESS;

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
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n",filename);
        return VX_FAILURE;
    }

    status |= vxQueryImage(input, VX_IMAGE_WIDTH,  &width,  sizeof(width));
    status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
    status |= vxQueryImage(input, VX_IMAGE_PLANES, &planes, sizeof(planes));
    status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

    status |= vxGetValidRegionImage(input, &rect);

    sx = rect.start_x;
    sy = rect.start_y;
    ex = rect.end_x;
    ey = rect.end_y;

    ext = strrchr(filename, '.');
    if (ext && (strcmp(ext, ".pgm") == 0 || strcmp(ext, ".PGM") == 0))
    {
        fprintf(fp, "P5\n# %s\n",filename);
        fprintf(fp, "%u %u\n", width, height);

        if (format == VX_DF_IMAGE_U8)
            fprintf(fp, "255\n");
        else if (format == VX_DF_IMAGE_S16)
            fprintf(fp, "65535\n");
        else if (format == VX_DF_IMAGE_U16)
            fprintf(fp, "65535\n");
    }

    for (p = 0u; p < planes; p++)
    {
        status |= vxMapImagePatch(input, &rect, p, &plane_map_id[p], &addr[p], (void**)&src[p], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    }

    for (p = 0u; (p < planes) && (status == VX_SUCCESS); p++)
    {
        size_t len = addr[p].stride_x * (addr[p].dim_x * addr[p].scale_x)/VX_SCALE_UNITY;

        for (y = 0u; y < height; y += addr[p].step_y)
        {
            vx_uint32 i = 0;
            vx_uint8* ptr = NULL;
            uint8_t value = 0u;

            if (y < sy || y >= ey)
            {
                for (i = 0; i < width; ++i)
                {
                    wrote += fwrite(&value, sizeof(value), 1, fp);
                }
                continue;
            }

            for (i = 0; i < sx; ++i)
                wrote += fwrite(&value, sizeof(value), 1, fp);

            ptr = vxFormatImagePatchAddress2d(src[p], 0, y - sy, &addr[p]);
            wrote += fwrite(ptr, 1, len, fp);

            for (i = 0; i < width - ex; ++i)
                wrote += fwrite(&value, sizeof(value), 1, fp);
        }

        if (wrote == 0)
        {
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to write to file!\n");
            status = VX_FAILURE;
            break;
        }

        if (status == VX_FAILURE)
        {
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to write image to file correctly\n");
            break;
        }
    }

    for (p = 0u; p < planes; p++)
    {
        status |= vxUnmapImagePatch(input, plane_map_id[p]);
    }

    if (status != VX_SUCCESS)
    {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to write image to file correctly\n");
    }

    fflush(fp);
    fclose(fp);

    if (vxUnmapArrayRange(file, fname_map_id) != VX_SUCCESS)
    {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to release handle to filename array!\n");
    }

    return status;
} /* ownFWriteImage() */

vx_status ownFReadImage(vx_array file, vx_image output)
{
    vx_char* filename = NULL;
    vx_size filename_stride = 0;
    vx_uint8* src = NULL;
    vx_uint32 p = 0u;
    vx_uint32 y = 0u;
    vx_size planes = 0u;
    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_df_image format = VX_DF_IMAGE_VIRT;
    FILE* fp = NULL;
    vx_char tmp[VX_MAX_FILE_NAME] = { 0 };
    vx_char* ext = NULL;
    vx_rectangle_t rect;
    vx_uint32 width = 0;
    vx_uint32 height = 0;
    vx_map_id fname_map_id = 0;
    vx_map_id image_map_id = 0;
    vx_status status = VX_SUCCESS;

    status = vxMapArrayRange(file, 0, VX_MAX_FILE_NAME, &fname_map_id, &filename_stride, (void**)&filename, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    if (status != VX_SUCCESS || filename_stride != sizeof(vx_char))
    {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
        return VX_FAILURE;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n",filename);
        return VX_FAILURE;
    }

    status |= vxQueryImage(output, VX_IMAGE_PLANES, &planes, sizeof(planes));
    status |= vxQueryImage(output, VX_IMAGE_FORMAT, &format, sizeof(format));

    ext = strrchr(filename, '.');
    if (ext && (strcmp(ext, ".pgm") == 0 || strcmp(ext, ".PGM") == 0))
    {
        FGETS(tmp, fp); // PX
        FGETS(tmp, fp); // comment
        FGETS(tmp, fp); // W H
        sscanf(tmp, "%u %u", &width, &height);
        FGETS(tmp, fp); // BPP
        // ! \todo double check image size?
    }
    else if (ext && (strcmp(ext, ".yuv") == 0 ||
                     strcmp(ext, ".rgb") == 0 ||
                     strcmp(ext, ".bw") == 0))
    {
        sscanf(filename, "%*[^_]_%ux%u_%*s", &width, &height);
    }

    rect.start_x = 0;
    rect.start_y = 0;
    rect.end_x   = width;
    rect.end_y   = height;

    for (p = 0; p < planes; p++)
    {
        src = NULL;
        status |= vxMapImagePatch(output, &rect, p, &image_map_id, &addr, (void**)&src, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        if (VX_SUCCESS == status)
        {
            for (y = 0; y < addr.dim_y; y += addr.step_y)
            {
                vx_uint8* srcp = vxFormatImagePatchAddress2d(src, 0, y, &addr);
                vx_size len = ((addr.dim_x * addr.scale_x)/VX_SCALE_UNITY);
                vx_size rlen = fread(srcp, addr.stride_x, len, fp);
                if (rlen != len)
                {
                    status = VX_FAILURE;
                    break;
                }
            }

            if (VX_SUCCESS == status)
            {
                status |= vxUnmapImagePatch(output, image_map_id);
            }

            if (VX_SUCCESS != status)
            {
                break;
            }
        }
    }

    fclose(fp);
    vxUnmapArrayRange(file, fname_map_id);

    return status;
} /* ownFReadImage() */

