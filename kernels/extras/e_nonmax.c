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
#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include "extras_k.h"

struct kp_elem
{
    int x;
    int y;
    vx_float32 resp;
};

void swap(struct kp_elem* x, struct kp_elem* y)
{
    struct kp_elem temp;
    temp = *x;
    *x = *y;
    *y = temp;
}

int choose_pivot(int i,int j )
{
    return((i+j) /2);
}

void quicksort(struct kp_elem list[], int m, int n)
{
    int i, j, k;
    struct kp_elem key;
    if ( m < n )
    {
        k = choose_pivot(m,n);
        swap(&list[m],&list[k]);
        key = list[m];
        i = m+1;
        j = n;
        while(i <= j)
        {
            while((i <= n) && (list[i].resp >= key.resp))
                i++;
            while((j >= m) && (list[j].resp < key.resp))
                j--;
            if( i < j)
                swap(&list[i],&list[j]);
        }
        /* swap two elements */
        swap(&list[m],&list[j]);

        /* recursively sort the lesser list */
        quicksort(list,m,j-1);
        quicksort(list,j+1,n);
    }
}

// nodeless version of the EuclideanNonMaxSuppression kernel
vx_status ownEuclideanNonMaxSuppressionHarris(vx_image src, vx_scalar thr, vx_scalar rad, vx_image dst)
{
    void* src_base = NULL;
    void* dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_float32 radius = 0.0f;
    vx_int32 r = 0;
    vx_float32 thresh = 0;
    vx_rectangle_t rect;
    vx_df_image format = VX_DF_IMAGE_VIRT;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(src, &rect);

    status |= vxCopyScalar(rad, &radius, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(thr, &thresh, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));

    status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

    r = (vx_int32)radius;
    r = (r <= 0 ? 1 : r);

    if (status == VX_SUCCESS)
    {
        vx_int32 x;
        vx_int32 y;
        vx_int32 i;
        vx_int32 j;
        vx_float32 d = 0;
        int n = 0;

        struct kp_elem* kp_list = (struct kp_elem*)malloc(src_addr.dim_x * src_addr.dim_y * sizeof(struct kp_elem));

        if (NULL != kp_list)
        {
            int nb_kp = 0;

            for (y = 0; y < (vx_int32)src_addr.dim_y; y++)
            {
                for (x = 0; x < (vx_int32)src_addr.dim_x; x++)
                {
                    // Init to 0
                    vx_int32 *out = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                    *out = 0;

                    // Fast non max suppression & keypoint list building
                    if ((x > 0) && (x < (vx_int32)src_addr.dim_x - 1) &&
                        (y > 0) && (y < (vx_int32)src_addr.dim_y - 1))
                    {
                        vx_float32* ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                        vx_float32* ptr99 = vxFormatImagePatchAddress2d(src_base, x - 1, y - 1, &src_addr);
                        vx_float32* ptr90 = vxFormatImagePatchAddress2d(src_base, x, y - 1, &src_addr);
                        vx_float32* ptr91 = vxFormatImagePatchAddress2d(src_base, x + 1, y - 1, &src_addr);
                        vx_float32* ptr09 = vxFormatImagePatchAddress2d(src_base, x - 1, y, &src_addr);
                        vx_float32* ptr01 = vxFormatImagePatchAddress2d(src_base, x + 1, y, &src_addr);
                        vx_float32* ptr19 = vxFormatImagePatchAddress2d(src_base, x - 1, y + 1, &src_addr);
                        vx_float32* ptr10 = vxFormatImagePatchAddress2d(src_base, x, y + 1, &src_addr);
                        vx_float32* ptr11 = vxFormatImagePatchAddress2d(src_base, x + 1, y + 1, &src_addr);

                        if ((*ptr >= thresh) &&
                            ((*ptr >= *ptr99) &&
                            (*ptr >= *ptr90) &&
                            (*ptr >= *ptr91) &&
                            (*ptr >= *ptr09) &&
                            (*ptr > *ptr01) &&
                            (*ptr > *ptr19) &&
                            (*ptr > *ptr10) &&
                            (*ptr > *ptr11))
                            )
                        {
                            kp_list[nb_kp].x = x;
                            kp_list[nb_kp].y = y;
                            kp_list[nb_kp].resp = *ptr;
                            nb_kp++;
                        }
                    }
                }
            }

            // Sort keypoints
            quicksort(kp_list, 0, nb_kp - 1);

            // Enclidean norm
            for (n = 0; n < nb_kp; n++)
            {
                int found = 0;
                x = kp_list[n].x;
                y = kp_list[n].y;

                for (j = -r; j <= r; j++)
                {
                    if ((y + j >= 0) && (y + j < (vx_int32)src_addr.dim_y))
                    {
                        for (i = -r; i <= r; i++)
                        {
                            if ((x + i >= 0) && (x + i < (vx_int32)src_addr.dim_x))
                            {
                                vx_float32 dx = (vx_float32)i;
                                vx_float32 dy = (vx_float32)j;

                                d = sqrtf((dx*dx) + (dy*dy));

                                if (d < radius)
                                {
                                    vx_float32* non = vxFormatImagePatchAddress2d(dst_base, x + i, y + j, &dst_addr);
                                    if (*non)
                                    {
                                        found = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if (found == 0)
                {
                    vx_float32* out = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                    *out = kp_list[n].resp;
                }
            }

            free(kp_list);
        }
        else
            status = VX_FAILURE;
    }

    status |= vxUnmapImagePatch(src, src_map_id);
    status |= vxUnmapImagePatch(dst, dst_map_id);

    return status;
} /* ownEuclideanNonMaxSuppressionHarris() */


static const int neighbor_indexes[][2] =
{
    { 3, 5 }, // 0
    { 6, 2 }, // 45
    { 7, 1 }, // 90
    { 8, 0 }, // 135
    { 5, 3 }, // 180
    { 2, 6 }, // 225
    { 1, 7 }, // 270
    { 0, 8 }, // 315
    { 3, 5 }, // 360
};

// nodeless version of the NonMaxSuppression kernel
vx_status ownNonMaxSuppression(vx_image i_mag, vx_image i_ang, vx_image i_edge, vx_border_t* borders)
{
    vx_uint32 x;
    vx_uint32 y;
    vx_uint32 low_x;
    vx_uint32 low_y;
    vx_uint32 high_x;
    vx_uint32 high_y;
    void* mag_base = NULL;
    void* ang_base = NULL;
    void* edge_base = NULL;
    vx_imagepatch_addressing_t mag_addr  = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t ang_addr  = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t edge_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id mag_map_id = 0;
    vx_map_id ang_map_id = 0;
    vx_map_id edge_map_id = 0;
    vx_rectangle_t rect;
    vx_df_image format = 0;
    vx_status status;

    status  = VX_SUCCESS; // assume success until an error occurs.

    status |= vxGetValidRegionImage(i_mag, &rect);

    status |= vxQueryImage(i_mag, VX_IMAGE_FORMAT, &format, sizeof(format));

    status |= vxMapImagePatch(i_mag, &rect, 0, &mag_map_id, &mag_addr, &mag_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(i_ang, &rect, 0, &ang_map_id, &ang_addr, &ang_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(i_edge, &rect, 0, &edge_map_id, &edge_addr, &edge_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

    low_x  = 0;
    low_y  = 0;
    high_x = edge_addr.dim_x;
    high_y = edge_addr.dim_y;

    if (borders->mode == VX_BORDER_UNDEFINED)
    {
        ++low_x; --high_x;
        ++low_y; --high_y;
        vxAlterRectangle(&rect, 1, 1, -1, -1);
    }

    for (y = low_y; y < high_y; y++)
    {
        for (x = low_x; x < high_x; x++)
        {
            vx_uint8* ang  = vxFormatImagePatchAddress2d(ang_base, x, y, &ang_addr);
            vx_uint8 angle = (vx_uint8)(127 - ang[0]); // shift range back after vxPhase
            vx_uint32 idx  = (angle + 16) / 32;
            const int* ni  = neighbor_indexes[idx];

            if (format == VX_DF_IMAGE_U8)
            {
                vx_uint8 mag[9];
                vx_uint8* edge = vxFormatImagePatchAddress2d(edge_base, x, y, &edge_addr);
                vxReadRectangle(mag_base, &mag_addr, borders, format, x, y, 1, 1, mag, 0);
                *edge = mag[4] > mag[ni[0]] && mag[4] > mag[ni[1]] ? mag[4] : 0;
            }
            else if (format == VX_DF_IMAGE_S16)
            {
                vx_int16 mag[9];
                vx_int16* edge = vxFormatImagePatchAddress2d(edge_base, x, y, &edge_addr);
                vxReadRectangle(mag_base, &mag_addr, borders, format, x, y, 1, 1, mag, 0);
                *edge = mag[4] > mag[ni[0]] && mag[4] > mag[ni[1]] ? mag[4] : 0;
            }
            else if (format == VX_DF_IMAGE_U16)
            {
                vx_uint16 mag[9];
                vx_uint16* edge = vxFormatImagePatchAddress2d(edge_base, x, y, &edge_addr);
                vxReadRectangle(mag_base, &mag_addr, borders, format, x, y, 1, 1, mag, 0);
                *edge = mag[4] > mag[ni[0]] && mag[4] > mag[ni[1]] ? mag[4] : 0;
            }
            else if (format == VX_DF_IMAGE_F32)
            {
                vx_float32 mag[9];
                vx_float32* edge = vxFormatImagePatchAddress2d(edge_base, x, y, &edge_addr);
                vxReadRectangle(mag_base, &mag_addr, borders, format, x, y, 1, 1, mag, 0);
                *edge = mag[4] > mag[ni[0]] && mag[4] > mag[ni[1]] ? mag[4] : 0;
            }
        }
    }

    status |= vxUnmapImagePatch(i_mag, mag_map_id);
    status |= vxUnmapImagePatch(i_ang, ang_map_id);
    status |= vxUnmapImagePatch(i_edge, edge_map_id);

    return status;
} /* ownNonMaxSuppression() */
