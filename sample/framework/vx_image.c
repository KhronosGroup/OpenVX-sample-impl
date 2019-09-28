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

#include <VX/vx.h>
#include <VX/vx_compatibility.h>

#include "vx_internal.h"
#include "vx_image.h"

static vx_uint32 vxComputePatchOffset(vx_uint32 x, vx_uint32 y, const vx_imagepatch_addressing_t* addr)
{
    return ( (addr->stride_y * ((addr->scale_y * y)/VX_SCALE_UNITY)) +
             (addr->stride_x * ((addr->scale_x * x)/VX_SCALE_UNITY)) );
}

static vx_uint32 vxComputePlaneOffset(vx_image image, vx_uint32 x, vx_uint32 y, vx_uint32 p)
{
    return ( ((y * image->memory.strides[p][VX_DIM_Y]) / image->scale[p][VX_DIM_Y]) +
             ((x * image->memory.strides[p][VX_DIM_X]) / image->scale[p][VX_DIM_X]) );
}

static vx_uint32 vxComputePatchRangeSize(vx_uint32 range, const vx_imagepatch_addressing_t* addr)
{
    return (range * addr->stride_x * addr->scale_x) / VX_SCALE_UNITY;
}

static vx_uint32 vxComputePlaneRangeSize(vx_image image, vx_uint32 range, vx_uint32 p)
{
    return (range * image->memory.strides[p][VX_DIM_X]) / image->scale[p][VX_DIM_X];
}

vx_bool ownIsValidImage(vx_image image)
{
    if ((ownIsValidSpecificReference(&image->base, VX_TYPE_IMAGE) == vx_true_e) &&
        (ownIsSupportedFourcc(image->format) == vx_true_e))
    {
        return vx_true_e;
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid Image!\n");
        return vx_false_e;
    }
}

vx_bool ownIsSupportedFourcc(vx_df_image code)
{
    switch (code)
    {
        case VX_DF_IMAGE_RGB:
        case VX_DF_IMAGE_RGBX:
        case VX_DF_IMAGE_NV12:
        case VX_DF_IMAGE_NV21:
        case VX_DF_IMAGE_UYVY:
        case VX_DF_IMAGE_YUYV:
        case VX_DF_IMAGE_IYUV:
        case VX_DF_IMAGE_YUV4:
        case VX_DF_IMAGE_U8:
        case VX_DF_IMAGE_U16:
        case VX_DF_IMAGE_S16:
        case VX_DF_IMAGE_U32:
        case VX_DF_IMAGE_S32:
        case VX_DF_IMAGE_F32:
        case VX_DF_IMAGE_VIRT:
            return vx_true_e;
        default:
            VX_PRINT(VX_ZONE_ERROR, "Format 0x%08x is not supported\n", code);
            if (code != 0) DEBUG_BREAK();
            return vx_false_e;
    }
}

static vx_size vxSizeOfChannel(vx_df_image color)
{
    vx_size size = 0ul;
    if (ownIsSupportedFourcc(color))
    {
        switch (color)
        {
            case VX_DF_IMAGE_S16:
            case VX_DF_IMAGE_U16:
                size = sizeof(vx_uint16);
                break;
            case VX_DF_IMAGE_U32:
            case VX_DF_IMAGE_S32:
            case VX_DF_IMAGE_F32:
                size = sizeof(vx_uint32);
                break;
            default:
                size = 1ul;
                break;
        }
    }
    return size;
}

void ownInitPlane(vx_image image,
                 vx_uint32 index,
                 vx_uint32 soc,
                 vx_uint32 channels,
                 vx_uint32 width,
                 vx_uint32 height)
{
    if (image)
    {
        image->memory.strides[index][VX_DIM_C] = soc;
        image->memory.dims[index][VX_DIM_C] = channels;
        image->memory.dims[index][VX_DIM_X] = width;
        image->memory.dims[index][VX_DIM_Y] = height;
        image->memory.ndims = VX_DIM_MAX;
        image->scale[index][VX_DIM_C] = 1;
        image->scale[index][VX_DIM_X] = 1;
        image->scale[index][VX_DIM_Y] = 1;
        image->bounds[index][VX_DIM_C][VX_BOUND_START] = 0;
        image->bounds[index][VX_DIM_C][VX_BOUND_END] = channels;
        image->bounds[index][VX_DIM_X][VX_BOUND_START] = 0;
        image->bounds[index][VX_DIM_X][VX_BOUND_END] = width;
        image->bounds[index][VX_DIM_Y][VX_BOUND_START] = 0;
        image->bounds[index][VX_DIM_Y][VX_BOUND_END] = height;
    }
}

void ownInitImage(vx_image image, vx_uint32 width, vx_uint32 height, vx_df_image color)
{
    vx_uint32 soc = (vx_uint32)vxSizeOfChannel(color);
    image->width = width;
    image->height = height;
    image->format = color;
    image->range = VX_CHANNEL_RANGE_FULL;
    image->memory_type = VX_MEMORY_TYPE_NONE;
    /* when an image is allocated, it's not valid until it's been written to.
     * this inverted rectangle is needed for the initial write case.
     */
    image->region.start_x = width;
    image->region.start_y = height;
    image->region.end_x = 0;
    image->region.end_y = 0;

    switch (image->format)
    {
        case VX_DF_IMAGE_U8:
        case VX_DF_IMAGE_U16:
        case VX_DF_IMAGE_U32:
        case VX_DF_IMAGE_S16:
        case VX_DF_IMAGE_S32:
        case VX_DF_IMAGE_F32:
            image->space = VX_COLOR_SPACE_NONE;
            break;
        default:
            image->space = VX_COLOR_SPACE_DEFAULT;
            break;
    }

    switch (image->format)
    {
        case VX_DF_IMAGE_VIRT:
            break;
        case VX_DF_IMAGE_NV12:
        case VX_DF_IMAGE_NV21:
            image->planes = 2;
            ownInitPlane(image, 0, soc, 1, image->width, image->height);
            ownInitPlane(image, 1, soc, 2, image->width/2, image->height/2);
            image->scale[1][VX_DIM_X] = 2;
            image->scale[1][VX_DIM_Y] = 2;
            image->bounds[1][VX_DIM_X][VX_BOUND_END] *= image->scale[1][VX_DIM_X];
            image->bounds[1][VX_DIM_Y][VX_BOUND_END] *= image->scale[1][VX_DIM_Y];
            break;
        case VX_DF_IMAGE_RGB:
            image->planes = 1;
            ownInitPlane(image, 0, soc, 3, image->width, image->height);
            break;
        case VX_DF_IMAGE_RGBX:
            image->planes = 1;
            ownInitPlane(image, 0, soc, 4, image->width, image->height);
            break;
        case VX_DF_IMAGE_UYVY:
        case VX_DF_IMAGE_YUYV:
            image->planes = 1;
            ownInitPlane(image, 0, soc, 2, image->width, image->height);
            break;
        case VX_DF_IMAGE_YUV4:
            image->planes = 3;
            ownInitPlane(image, 0, soc, 1, image->width, image->height);
            ownInitPlane(image, 1, soc, 1, image->width, image->height);
            ownInitPlane(image, 2, soc, 1, image->width, image->height);
            break;
        case VX_DF_IMAGE_IYUV:
            image->planes = 3;
            ownInitPlane(image, 0, soc, 1, image->width, image->height);
            ownInitPlane(image, 1, soc, 1, image->width/2, image->height/2);
            ownInitPlane(image, 2, soc, 1, image->width/2, image->height/2);
            image->scale[1][VX_DIM_X] = 2;
            image->scale[1][VX_DIM_Y] = 2;
            image->scale[2][VX_DIM_X] = 2;
            image->scale[2][VX_DIM_Y] = 2;
            image->bounds[1][VX_DIM_X][VX_BOUND_END] *= image->scale[1][VX_DIM_X];
            image->bounds[1][VX_DIM_Y][VX_BOUND_END] *= image->scale[1][VX_DIM_Y];
            image->bounds[2][VX_DIM_X][VX_BOUND_END] *= image->scale[2][VX_DIM_X];
            image->bounds[2][VX_DIM_Y][VX_BOUND_END] *= image->scale[2][VX_DIM_Y];
            break;
        case VX_DF_IMAGE_U8:
            image->planes = 1;
            ownInitPlane(image, 0, soc, 1, image->width, image->height);
            break;
        case VX_DF_IMAGE_U16:
        case VX_DF_IMAGE_S16:
            image->planes = 1;
            ownInitPlane(image, 0, soc, 1, image->width, image->height);
            break;
        case VX_DF_IMAGE_U32:
        case VX_DF_IMAGE_S32:
            image->planes = 1;
            ownInitPlane(image, 0, soc, 1, image->width, image->height);
            break;
        case VX_DF_IMAGE_F32:
            image->planes = 1;
            ownInitPlane(image, 0, soc, 1, image->width, image->height);
            break;
        default:
            /*! should not get here unless there's a bug in the
             * ownIsSupportedFourcc call.
             */
            VX_PRINT(VX_ZONE_ERROR, "#################################################\n");
            VX_PRINT(VX_ZONE_ERROR, "Unsupported IMAGE FORMAT!!!\n");
            VX_PRINT(VX_ZONE_ERROR, "#################################################\n");
            break;
    }
    image->memory.nptrs = image->planes;
    ownPrintImage(image);
}

void ownFreeImage(vx_image image)
{
    ownFreeMemory(image->base.context, &image->memory);
}

/* Allocate the memory for an image. The function returns vx_false_e in case
 * the image could not be allocated:
 * - No memory resource
 * - Handled reclaimed for images created from handle
*/
vx_bool ownAllocateImage(vx_image image)
{
    vx_bool ret = vx_true_e;

    /*
     * Note: ownAllocateMemory allocates memory when the 'memory->allocated' flag
     * is 'vx_false_e'. For images created from handle, this flag is set to
     * 'vx_false_e' when there is no handle and no memory allocation should be done
     * by OpenVX. ownAllocateMemory then must not be called for images created from
     * handle.
     */

    /* Only request allocation OpenVX controlled memory */
    if (image->memory_type == VX_MEMORY_TYPE_NONE)
    {
        /* Standard image */
        ret = ownAllocateMemory(image->base.context, &image->memory);
        ownPrintMemory(&image->memory);
    }
    else {
        /* This is an image created from handle */
        ret = image->memory.allocated;
    }

    return ret;
}

static vx_bool vxIsOdd(vx_uint32 a)
{
    if (a & 0x1)
        return vx_true_e;
    else
        return vx_false_e;
}

vx_bool vxIsValidDimensions(vx_uint32 width, vx_uint32 height, vx_df_image color)
{
    if (vxIsOdd(width) && (color == VX_DF_IMAGE_UYVY || color == VX_DF_IMAGE_YUYV))
    {
        return vx_false_e;
    }
    else if ((vxIsOdd(width) || vxIsOdd(height)) &&
              (color == VX_DF_IMAGE_IYUV || color == VX_DF_IMAGE_NV12 || color == VX_DF_IMAGE_NV21))
    {
        return vx_false_e;
    }
    return vx_true_e;
}

static vx_image_t *vxCreateImageInt(vx_context_t *context,
                                     vx_uint32 width,
                                     vx_uint32 height,
                                     vx_df_image color,
                                     vx_bool is_virtual)
{
    vx_image image = NULL;

    if (ownIsValidContext(context) == vx_true_e)
    {
        if (ownIsSupportedFourcc(color) == vx_true_e)
        {
            if (vxIsValidDimensions(width, height, color) == vx_true_e)
            {
                image = (vx_image)ownCreateReference(context, VX_TYPE_IMAGE, VX_EXTERNAL, &context->base);
                if (vxGetStatus((vx_reference)image) == VX_SUCCESS && image->base.type == VX_TYPE_IMAGE)
                {
                    image->base.is_virtual = is_virtual;
                    ownInitImage(image, width, height, color);
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Requested Image Dimensions are invalid!\n");
                vxAddLogEntry((vx_reference)image, VX_ERROR_INVALID_DIMENSION, "Requested Image Dimensions was invalid!\n");
                image = (vx_image_t *)ownGetErrorObject(context, VX_ERROR_INVALID_DIMENSION);
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Requested Image Format was invalid!\n");
            vxAddLogEntry((vx_reference)context, VX_ERROR_INVALID_FORMAT, "Requested Image Format was invalid!\n");
            image = (vx_image_t *)ownGetErrorObject(context, VX_ERROR_INVALID_FORMAT);
        }
    }

    return image;
}

VX_API_ENTRY vx_image VX_API_CALL vxCreateImage(vx_context context, vx_uint32 width, vx_uint32 height, vx_df_image format)
{
    if ((width == 0) || (height == 0) ||
        (ownIsSupportedFourcc(format) == vx_false_e) || (format == VX_DF_IMAGE_VIRT))
    {
        return (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
    }
    return (vx_image)vxCreateImageInt(context, width, height, format, vx_false_e);
}

VX_API_ENTRY vx_image VX_API_CALL vxCreateUniformImage(vx_context context, vx_uint32 width, vx_uint32 height, vx_df_image format, const vx_pixel_value_t *value)
{
    vx_image image = 0;

    if (value == NULL)
        return (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);

    image = vxCreateImage(context, width, height, format);
    if (vxGetStatus((vx_reference)image) == VX_SUCCESS)
    {
        vx_uint32 x, y, p;
        vx_size planes = 0;
        vx_rectangle_t rect = {0, 0, width, height};
        vxQueryImage(image, VX_IMAGE_PLANES, &planes, sizeof(planes));
        for (p = 0; p < planes; p++)
        {
            vx_imagepatch_addressing_t addr;
            void *base = NULL;
            if (vxAccessImagePatch(image, &rect, p, &addr, &base, VX_WRITE_ONLY) == VX_SUCCESS)
            {
                ownPrintImageAddressing(&addr);
                for (y = 0; y < addr.dim_y; y+=addr.step_y)
                {
                    for (x = 0; x < addr.dim_x; x+=addr.step_x)
                    {
                        if (format == VX_DF_IMAGE_U8)
                        {
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = value->U8;
                        }
                        else if (format == VX_DF_IMAGE_U16)
                        {
                            vx_uint16 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = value->U16;
                        }
                        else if (format == VX_DF_IMAGE_U32)
                        {
                            vx_uint32 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = value->U32;
                        }
                        else if (format == VX_DF_IMAGE_S16)
                        {
                            vx_int16 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = value->S16;
                        }
                        else if (format == VX_DF_IMAGE_S32)
                        {
                            vx_int32 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = value->S32;
                        }
                        else if ((format == VX_DF_IMAGE_RGB)  ||
                                 (format == VX_DF_IMAGE_RGBX))
                        {
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            ptr[0] = value->RGBX[0];
                            ptr[1] = value->RGBX[1];
                            ptr[2] = value->RGBX[2];
                            if (format == VX_DF_IMAGE_RGBX)
                                ptr[3] = value->RGBX[3];
                        }
                        else if ((format == VX_DF_IMAGE_YUV4) ||
                                 (format == VX_DF_IMAGE_IYUV))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel[p];
                        }
                        else if ((p == 0) &&
                                 ((format == VX_DF_IMAGE_NV12) ||
                                  (format == VX_DF_IMAGE_NV21)))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel[0];
                        }
                        else if ((p == 1) && (format == VX_DF_IMAGE_NV12))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            ptr[0] = pixel[1];
                            ptr[1] = pixel[2];
                        }
                        else if ((p == 1) && (format == VX_DF_IMAGE_NV21))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            ptr[0] = pixel[2];
                            ptr[1] = pixel[1];
                        }
                        else if (format == VX_DF_IMAGE_UYVY)
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            if (x % 2 == 0)
                            {
                                ptr[0] = pixel[1];
                                ptr[1] = pixel[0];
                            }
                            else
                            {
                                ptr[0] = pixel[2];
                                ptr[1] = pixel[0];
                            }
                        }
                        else if (format == VX_DF_IMAGE_YUYV)
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            if (x % 2 == 0)
                            {
                                ptr[0] = pixel[0];
                                ptr[1] = pixel[1];
                            }
                            else
                            {
                                ptr[0] = pixel[0];
                                ptr[1] = pixel[2];
                            }
                        }
                    }
                }
                if (vxCommitImagePatch(image, &rect, p, &addr, base) != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR, "Failed to set initial image patch on plane %u on const image!\n", p);
                    vxReleaseImage(&image);
                    image = (vx_image)ownGetErrorObject(context, VX_FAILURE);
                    break;
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Failed to get image patch on plane %u in const image!\n",p);
                vxReleaseImage(&image);
                image = (vx_image)ownGetErrorObject(context, VX_FAILURE);
                break;
            }
        } /* for loop */
        if (vxGetStatus((vx_reference)image) == VX_SUCCESS)
        {
            /* lock the image from being modified again! */
            ((vx_image_t *)image)->constant = vx_true_e;
        }
    }

    return image;
}

VX_API_ENTRY vx_image VX_API_CALL vxCreateVirtualImage(vx_graph graph, vx_uint32 width, vx_uint32 height, vx_df_image format)
{
    vx_image image = NULL;
    vx_reference_t *gref = (vx_reference_t *)graph;

    if (ownIsValidSpecificReference(gref, VX_TYPE_GRAPH) == vx_true_e)
    {
        image = vxCreateImageInt(gref->context, width, height, format, vx_true_e);
        if (vxGetStatus((vx_reference)image) == VX_SUCCESS && image->base.type == VX_TYPE_IMAGE)
        {
            image->base.scope = (vx_reference_t *)graph;
        }
    }

    return image;
}

VX_API_ENTRY vx_image VX_API_CALL vxCreateImageFromROI(vx_image image, const vx_rectangle_t* rect)
{
    vx_image_t* subimage = NULL;

    if (ownIsValidImage(image) == vx_true_e)
    {
        if (!rect ||
            rect->start_x > rect->end_x ||
            rect->start_y > rect->end_y ||
            rect->end_x > image->width ||
            rect->end_y > image->height)
        {
            vx_context context = vxGetContext((vx_reference)image);
            subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
        }
        else
        {
            /* perhaps the parent hasn't been allocated yet? */
            if (ownAllocateImage(image) == vx_true_e)
            {
                subimage = (vx_image)ownCreateReference(image->base.context, VX_TYPE_IMAGE, VX_EXTERNAL, &image->base.context->base);
                if (vxGetStatus((vx_reference)subimage) == VX_SUCCESS)
                {
                    vx_uint32 p = 0;
                    vx_rectangle_t image_rect;

                    /* remember that the scope of the image is the parent image */
                    subimage->base.scope = (vx_reference_t*)image;

                    /* refer to our parent image and internally refcount it */
                    subimage->parent = image;

                    for (p = 0; p < VX_INT_MAX_REF; p++)
                    {
                        if (image->subimages[p] == NULL)
                        {
                            image->subimages[p] = subimage;
                            break;
                        }
                    }

                    ownIncrementReference(&image->base, VX_INTERNAL);

                    VX_PRINT(VX_ZONE_IMAGE, "Creating SubImage at {%u,%u},{%u,%u}\n",
                             rect->start_x, rect->start_y, rect->end_x, rect->end_y);

                    /* duplicate the metadata */
                    subimage->format      = image->format;
                    subimage->memory_type = image->memory_type;
                    subimage->range       = image->range;
                    subimage->space       = image->space;
                    subimage->width       = rect->end_x - rect->start_x;
                    subimage->height      = rect->end_y - rect->start_y;
                    subimage->planes      = image->planes;
                    subimage->constant    = image->constant;

                    vxGetValidRegionImage(image, &image_rect);

                    /* set valid rectangle */
                    if(rect->start_x > image_rect.end_x ||
                       rect->end_x   < image_rect.start_x ||
                       rect->start_y > image_rect.end_y ||
                       rect->end_y   < image_rect.start_y)
                    {
                        /* no intersection */
                        subimage->region.start_x = 0;
                        subimage->region.start_y = 0;
                        subimage->region.end_x   = 0;
                        subimage->region.end_y   = 0;
                    }
                    else
                    {
                        subimage->region.start_x = VX_MAX(image_rect.start_x, rect->start_x) - rect->start_x;
                        subimage->region.start_y = VX_MAX(image_rect.start_y, rect->start_y) - rect->start_y;
                        subimage->region.end_x   = VX_MIN(image_rect.end_x,   rect->end_x)   - rect->start_x;
                        subimage->region.end_y   = VX_MIN(image_rect.end_y,   rect->end_y)   - rect->start_y;
                    }

                    memcpy(&subimage->scale, &image->scale, sizeof(image->scale));
                    memcpy(&subimage->memory, &image->memory, sizeof(image->memory));

                    /* modify the dimensions */
                    for (p = 0; p < subimage->planes; p++)
                    {
                        vx_uint32 offset = vxComputePlaneOffset(image, rect->start_x, rect->start_y, p);
                        VX_PRINT(VX_ZONE_IMAGE, "Offsetting SubImage plane[%u] by %u bytes!\n", p, offset);

                        subimage->memory.dims[p][VX_DIM_X] = subimage->width;
                        subimage->memory.dims[p][VX_DIM_Y] = subimage->height;
                        subimage->memory.ptrs[p] = &image->memory.ptrs[p][offset];

                        /* keep offset to allow vxSwapImageHandle update ROI pointers */
                        subimage->memory.offset[p] = offset;
                    }

                    ownPrintImage(subimage);
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Child image failed to allocate!\n");
                }
            }
            else
            {
                vx_context context;
                VX_PRINT(VX_ZONE_ERROR, "Parent image failed to allocate!\n");
                context = vxGetContext((vx_reference)image);
                subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_NO_MEMORY);
            }
        }
    }
    else
    {
        vx_context context = vxGetContext((vx_reference)image);
        subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
    }

    return (vx_image)subimage;
}

VX_API_ENTRY vx_image VX_API_CALL vxCreateImageFromChannel(vx_image image, vx_enum channel)
{
    vx_image_t* subimage = NULL;

    if (ownIsValidImage(image) == vx_true_e)
    {
        /* perhaps the parent hasn't been allocated yet? */
        if (ownAllocateImage(image) == vx_true_e)
        {
            /* check for valid parameters */
            switch (channel)
            {
                case VX_CHANNEL_Y:
                {
                    if (VX_DF_IMAGE_YUV4 != image->format &&
                        VX_DF_IMAGE_IYUV != image->format &&
                        VX_DF_IMAGE_NV12 != image->format &&
                        VX_DF_IMAGE_NV21 != image->format)
                    {
                        vx_context context = vxGetContext((vx_reference)image);
                        subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
                        return subimage;
                    }
                    break;
                }

                case VX_CHANNEL_U:
                case VX_CHANNEL_V:
                {
                    if (VX_DF_IMAGE_YUV4 != image->format &&
                        VX_DF_IMAGE_IYUV != image->format)
                    {
                        vx_context context = vxGetContext((vx_reference)image);
                        subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
                        return subimage;
                    }
                    break;
                }

                default:
                {
                    vx_context context = vxGetContext((vx_reference)image);
                    subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
                    return subimage;
                }
            }

            subimage = (vx_image)ownCreateReference(image->base.context, VX_TYPE_IMAGE, VX_EXTERNAL, &image->base.context->base);
            if (vxGetStatus((vx_reference)subimage) == VX_SUCCESS)
            {
                vx_uint32 p = 0;

                /* remember that the scope of the subimage is the parent image */
                subimage->base.scope = (vx_reference_t*)image;

                /* refer to our parent image and internally refcount it */
                subimage->parent = image;

                for (p = 0; p < VX_INT_MAX_REF; p++)
                {
                    if (image->subimages[p] == NULL)
                    {
                        image->subimages[p] = subimage;
                        break;
                    }
                }

                ownIncrementReference(&image->base, VX_INTERNAL);

                VX_PRINT(VX_ZONE_IMAGE, "Creating SubImage from channel {%u}\n", channel);

                /* plane index */
                p = (VX_CHANNEL_Y == channel) ? 0 : ((VX_CHANNEL_U == channel) ? 1 : 2);

                switch (image->format)
                {
                    case VX_DF_IMAGE_YUV4:
                    {
                        /* setup the metadata */
                        subimage->format      = VX_DF_IMAGE_U8;
                        subimage->memory_type = image->memory_type;
                        subimage->range       = image->range;
                        subimage->space       = image->space;
                        subimage->width       = image->memory.dims[p][VX_DIM_X];
                        subimage->height      = image->memory.dims[p][VX_DIM_Y];
                        subimage->planes      = 1;
                        subimage->constant    = image->constant;

                        memset(&subimage->scale, 0, sizeof(image->scale));
                        memset(&subimage->memory, 0, sizeof(image->memory));

                        subimage->scale[0][VX_DIM_C] = 1;
                        subimage->scale[0][VX_DIM_X] = 1;
                        subimage->scale[0][VX_DIM_Y] = 1;

                        subimage->bounds[0][VX_DIM_C][VX_BOUND_START] = 0;
                        subimage->bounds[0][VX_DIM_C][VX_BOUND_END]   = 1;
                        subimage->bounds[0][VX_DIM_X][VX_BOUND_START] = image->bounds[p][VX_DIM_X][VX_BOUND_START];
                        subimage->bounds[0][VX_DIM_X][VX_BOUND_END]   = image->bounds[p][VX_DIM_X][VX_BOUND_END];
                        subimage->bounds[0][VX_DIM_Y][VX_BOUND_START] = image->bounds[p][VX_DIM_Y][VX_BOUND_START];
                        subimage->bounds[0][VX_DIM_Y][VX_BOUND_END]   = image->bounds[p][VX_DIM_Y][VX_BOUND_END];

                        subimage->memory.dims[0][VX_DIM_C] = image->memory.dims[p][VX_DIM_C];
                        subimage->memory.dims[0][VX_DIM_X] = image->memory.dims[p][VX_DIM_X];
                        subimage->memory.dims[0][VX_DIM_Y] = image->memory.dims[p][VX_DIM_Y];

                        subimage->memory.strides[0][VX_DIM_C] = image->memory.strides[p][VX_DIM_C];
                        subimage->memory.strides[0][VX_DIM_X] = image->memory.strides[p][VX_DIM_X];
                        subimage->memory.strides[0][VX_DIM_Y] = image->memory.strides[p][VX_DIM_Y];

                        subimage->memory.ptrs[0] = image->memory.ptrs[p];
                        subimage->memory.nptrs   = 1;

                        ownCreateSem(&subimage->memory.locks[0], 1);

                        break;
                    }

                    case VX_DF_IMAGE_IYUV:
                    case VX_DF_IMAGE_NV12:
                    case VX_DF_IMAGE_NV21:
                    {
                        /* setup the metadata */
                        subimage->format      = VX_DF_IMAGE_U8;
                        subimage->memory_type = image->memory_type;
                        subimage->range       = image->range;
                        subimage->space       = image->space;
                        subimage->width       = image->memory.dims[p][VX_DIM_X];
                        subimage->height      = image->memory.dims[p][VX_DIM_Y];
                        subimage->planes      = 1;
                        subimage->constant    = image->constant;

                        memset(&subimage->scale, 0, sizeof(image->scale));
                        memset(&subimage->memory, 0, sizeof(image->memory));

                        subimage->scale[0][VX_DIM_C] = 1;
                        subimage->scale[0][VX_DIM_X] = 1;
                        subimage->scale[0][VX_DIM_Y] = 1;

                        subimage->bounds[0][VX_DIM_C][VX_BOUND_START] = 0;
                        subimage->bounds[0][VX_DIM_C][VX_BOUND_END]   = 1;
                        subimage->bounds[0][VX_DIM_X][VX_BOUND_START] = image->bounds[p][VX_DIM_X][VX_BOUND_START];
                        subimage->bounds[0][VX_DIM_X][VX_BOUND_END]   = image->bounds[p][VX_DIM_X][VX_BOUND_END];
                        subimage->bounds[0][VX_DIM_Y][VX_BOUND_START] = image->bounds[p][VX_DIM_Y][VX_BOUND_START];
                        subimage->bounds[0][VX_DIM_Y][VX_BOUND_END]   = image->bounds[p][VX_DIM_Y][VX_BOUND_END];

                        subimage->memory.dims[0][VX_DIM_C] = image->memory.dims[p][VX_DIM_C];
                        subimage->memory.dims[0][VX_DIM_X] = image->memory.dims[p][VX_DIM_X];
                        subimage->memory.dims[0][VX_DIM_Y] = image->memory.dims[p][VX_DIM_Y];

                        subimage->memory.strides[0][VX_DIM_C] = image->memory.strides[p][VX_DIM_C];
                        subimage->memory.strides[0][VX_DIM_X] = image->memory.strides[p][VX_DIM_X];
                        subimage->memory.strides[0][VX_DIM_Y] = image->memory.strides[p][VX_DIM_Y];

                        subimage->memory.ptrs[0] = image->memory.ptrs[p];
                        subimage->memory.nptrs = 1;

                        ownCreateSem(&subimage->memory.locks[0], 1);

                        break;
                    }
                }

                /* set inverted region untill the first write to image */
                subimage->region.start_x = subimage->width;
                subimage->region.start_y = subimage->height;
                subimage->region.end_x   = 0;
                subimage->region.end_y   = 0;

                ownPrintImage(subimage);
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Child image failed to allocate!\n");
            }
        }
        else
        {
            vx_context context;
            VX_PRINT(VX_ZONE_ERROR, "Parent image failed to allocate!\n");
            context = vxGetContext((vx_reference)image);
            subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_NO_MEMORY);
        }
    }
    else
    {
        vx_context context = vxGetContext((vx_reference)subimage);
        subimage = (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
    }

    return (vx_image)subimage;
}

VX_API_ENTRY vx_image VX_API_CALL vxCreateImageFromHandle(vx_context context, vx_df_image color, const vx_imagepatch_addressing_t addrs[], void *const ptrs[], vx_enum memory_type)
{
    vx_image image = 0;

    if (ownIsValidImport(memory_type) == vx_false_e)
        return (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);

    image = vxCreateImage(context, addrs[0].dim_x, addrs[0].dim_y, color);

    if (vxGetStatus((vx_reference)image) == VX_SUCCESS && image->base.type == VX_TYPE_IMAGE)
    {
        vx_uint32 p = 0;
        image->memory_type = memory_type;
        image->memory.allocated = vx_true_e; /* don't let the system realloc this memory */

        /* now assign the plane pointers, assume linearity */
        for (p = 0; p < image->planes; p++)
        {
            /* ensure row-major memory layout */
            if (addrs[p].stride_x <= 0 || addrs[p].stride_y < (vx_int32)(addrs[p].stride_x * addrs[p].dim_x))
            {
                vxReleaseImage(&image);
                return (vx_image)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
            }
#ifdef OPENVX_USE_OPENCL_INTEROP
            //clEnqueueReadBuffer(context->opencl_command_queue, ptrs[p], CL_TRUE, 0, sizeof(vx_uint8) * sizeof(*ptrs[p]), (void *) image->memory.ptrs[p], 0, NULL, NULL);
            image->memory.ptrs[p] = ptrs[p];
#else
            image->memory.ptrs[p] = ptrs[p];
#endif
            image->memory.strides[p][VX_DIM_C] = (vx_uint32)vxSizeOfChannel(color);
            image->memory.strides[p][VX_DIM_X] = addrs[p].stride_x;
            image->memory.strides[p][VX_DIM_Y] = addrs[p].stride_y;

            ownCreateSem(&image->memory.locks[p], 1);
        }
    }

    return image;
}

VX_API_ENTRY vx_status VX_API_CALL vxSwapImageHandle(vx_image image, void* const new_ptrs[],
    void* prev_ptrs[], vx_size num_planes)
{
    vx_status status = VX_SUCCESS;
    (void)num_planes;

    if (ownIsValidImage(image) == vx_true_e)
    {
        if (image->memory_type != VX_MEMORY_TYPE_NONE)
        {
            vx_uint32 i;
            vx_uint32 p;

            if (new_ptrs != NULL)
            {
                for (p = 0; p < image->planes; p++)
                {
                    if (new_ptrs[p] == NULL)
                        return VX_ERROR_INVALID_PARAMETERS;
                }
            }

            if (prev_ptrs != NULL && image->parent != NULL)
            {
                /* do not return prev pointers for subimages */
                return VX_ERROR_INVALID_PARAMETERS;
            }

            if (prev_ptrs != NULL && image->parent == NULL)
            {
                /* return previous image handles */
                for (p = 0; p < image->planes; p++)
                {
                    prev_ptrs[p] = image->memory.ptrs[p];
                }
            }

            /* visit each subimage of this image and reclaim its pointers */
            for (i = 0; i < VX_INT_MAX_REF; i++)
            {
                if (image->subimages[i] != NULL)
                {
                    if (new_ptrs == NULL)
                        status = vxSwapImageHandle(image->subimages[i], NULL, NULL, image->planes);
                    else
                    {
                        vx_uint8* ptrs[4];

                        for (p = 0; p < image->subimages[i]->planes; p++)
                        {
                            vx_uint32 offset = image->subimages[i]->memory.offset[p];
                            ptrs[p] = (vx_uint8*)new_ptrs[p] + offset;
                        }

                        status = vxSwapImageHandle(image->subimages[i], (void**)ptrs, NULL, image->planes);
                    }

                    break;
                }
            }

            /* reclaim previous and set new handles for this image */
            for (p = 0; p < image->planes; p++)
            {
                if (new_ptrs == NULL)
                    image->memory.ptrs[p] = 0;
                else
                {
                    /* set new pointers for subimage */
                    image->memory.ptrs[p] = new_ptrs[p];
                }
            }

            /* clear flag if pointers were reclaimed */
            image->memory.allocated = (new_ptrs == NULL) ? vx_false_e : vx_true_e;
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryImage(vx_image image, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidImage(image) == vx_true_e)
    {
        switch (attribute)
        {
            case VX_IMAGE_FORMAT:
                if (VX_CHECK_PARAM(ptr, size, vx_df_image, 0x3))
                {
                    *(vx_df_image *)ptr = image->format;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMAGE_WIDTH:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = image->width;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMAGE_HEIGHT:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = image->height;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMAGE_PLANES:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = image->planes;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMAGE_SPACE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = image->space;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMAGE_RANGE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = image->range;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMAGE_SIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    vx_size size = 0ul;
                    vx_uint32 p;
                    for (p = 0; p < image->planes; p++)
                    {
                        size += (abs(image->memory.strides[p][VX_DIM_Y]) * image->memory.dims[p][VX_DIM_Y]);
                    }
                    *(vx_size *)ptr = size;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMAGE_MEMORY_TYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = image->memory_type;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    VX_PRINT(VX_ZONE_API, "%s returned %d\n", __FUNCTION__, status);
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetImageAttribute(vx_image image, vx_enum attribute, const void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidImage(image) == vx_true_e)
    {
        switch (attribute)
        {
            case VX_IMAGE_SPACE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    image->space = *(vx_enum *)ptr;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;

            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }
    else
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    VX_PRINT(VX_ZONE_API, "%s returned %d\n", __FUNCTION__, status);
    return status;
}

void ownDestructImage(vx_reference ref)
{
    vx_image image = (vx_image)ref;
    /* if it's not imported and does not have a parent, free it */
    if ((image->memory_type == VX_MEMORY_TYPE_NONE) && (image->parent == NULL))
    {
        ownFreeImage(image);
    }
    else if (image->parent)
    {
        ownReleaseReferenceInt((vx_reference *)&image->parent, VX_TYPE_IMAGE, VX_INTERNAL, NULL);
    }
    else if (image->memory_type != VX_MEMORY_TYPE_NONE)
    {
        vx_uint32 p = 0u;
        for (p = 0; p < image->planes; p++)
        {
            ownDestroySem(&image->memory.locks[p]);
            image->memory.ptrs[p] = NULL;
            image->memory.strides[p][VX_DIM_C] = 0;
            image->memory.strides[p][VX_DIM_X] = 0;
            image->memory.strides[p][VX_DIM_Y] = 0;
        }
        image->memory.allocated = vx_false_e;
    }
}

VX_API_ENTRY vx_status VX_API_CALL vxSetImagePixelValues(vx_image image, const vx_pixel_value_t *pixel_value)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidImage(image) == vx_true_e)
    {
        vx_uint32 x, y, p;
        vx_size planes = 0;
        vx_rectangle_t rect = {0,0,0,0};
        vxGetValidRegionImage(image, &rect);
        vx_df_image format = 0;
        vxQueryImage(image, VX_IMAGE_FORMAT, &format, sizeof(format));
        vxQueryImage(image, VX_IMAGE_PLANES, &planes, sizeof(planes));
        for (p = 0; p < planes; p++)
        {
            vx_imagepatch_addressing_t addr;
            void *base = NULL;
            if (vxAccessImagePatch(image, &rect, p, &addr, &base, VX_WRITE_ONLY) == VX_SUCCESS)
            {
                ownPrintImageAddressing(&addr);
                for (y = 0; y < addr.dim_y; y+=addr.step_y)
                {
                    for (x = 0; x < addr.dim_x; x+=addr.step_x)
                    {
                        if (format == VX_DF_IMAGE_U8)
                        {
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel_value->U8;
                        }
                        else if (format == VX_DF_IMAGE_U16)
                        {
                            vx_uint16 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel_value->U16;
                        }
                        else if (format == VX_DF_IMAGE_U32)
                        {
                            vx_uint32 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel_value->U32;
                        }
                        else if (format == VX_DF_IMAGE_S16)
                        {
                            vx_int16 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel_value->S16;
                        }
                        else if (format == VX_DF_IMAGE_S32)
                        {
                            vx_int32 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel_value->S32;
                        }
                        else if ((format == VX_DF_IMAGE_RGB)  ||
                                 (format == VX_DF_IMAGE_RGBX))
                        {
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            ptr[0] = pixel_value->RGBX[0];
                            ptr[1] = pixel_value->RGBX[1];
                            ptr[2] = pixel_value->RGBX[2];
                            if (format == VX_DF_IMAGE_RGBX)
                                ptr[3] = pixel_value->RGBX[3];
                        }
                        else if ((format == VX_DF_IMAGE_YUV4) ||
                                 (format == VX_DF_IMAGE_IYUV))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&pixel_value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel[p];
                        }
                        else if ((p == 0) &&
                                 ((format == VX_DF_IMAGE_NV12) ||
                                  (format == VX_DF_IMAGE_NV21)))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&pixel_value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            *ptr = pixel[0];
                        }
                        else if ((p == 1) && (format == VX_DF_IMAGE_NV12))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&pixel_value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            ptr[0] = pixel[1];
                            ptr[1] = pixel[2];
                        }
                        else if ((p == 1) && (format == VX_DF_IMAGE_NV21))
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&pixel_value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            ptr[0] = pixel[2];
                            ptr[1] = pixel[1];
                        }
                        else if (format == VX_DF_IMAGE_UYVY)
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&pixel_value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            if (x % 2 == 0)
                            {
                                ptr[0] = pixel[1];
                                ptr[1] = pixel[0];
                            }
                            else
                            {
                                ptr[0] = pixel[2];
                                ptr[1] = pixel[0];
                            }
                        }
                        else if (format == VX_DF_IMAGE_YUYV)
                        {
                            vx_uint8 *pixel = (vx_uint8 *)&pixel_value->YUV;
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            if (x % 2 == 0)
                            {
                                ptr[0] = pixel[0];
                                ptr[1] = pixel[1];
                            }
                            else
                            {
                                ptr[0] = pixel[0];
                                ptr[1] = pixel[2];
                            }
                        }
                    }
                }
                if (vxCommitImagePatch(image, &rect, p, &addr, base) != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR, "Failed to set initial image patch on plane %u on const image!\n", p);
                    status = VX_FAILURE;
                    break;
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Failed to get image patch on plane %u in const image!\n",p);
                status = VX_FAILURE;
                break;
            }
        } /* for loop */
    }
    else
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseImage(vx_image* image)
{
    if (image != NULL)
    {
        vx_image this_image = image[0];
        if (ownIsValidSpecificReference((vx_reference)this_image, VX_TYPE_IMAGE) == vx_true_e)
        {
            vx_image parent = this_image->parent;

            /* clear this image from its parent' subimages list */
            if (parent && ownIsValidSpecificReference((vx_reference)parent, VX_TYPE_IMAGE) == vx_true_e)
            {
                vx_uint32 n;
                for (n = 0; n < VX_INT_MAX_REF; n++)
                {
                    if (parent->subimages[n] == this_image)
                    {
                        parent->subimages[n] = 0;
                        break;
                    }
                }
            }
        }
    }

    return ownReleaseReferenceInt((vx_reference *)image, VX_TYPE_IMAGE, VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_size VX_API_CALL vxComputeImagePatchSize(vx_image image,
                                       const vx_rectangle_t *rect,
                                       vx_uint32 plane_index)
{
    vx_size size = 0ul;
    vx_uint32 start_x = 0u, start_y = 0u, end_x = 0u, end_y = 0u;

    if ((ownIsValidImage(image) == vx_true_e) && (rect))
    {
        start_x = rect->start_x;
        start_y = rect->start_y;
        end_x = rect->end_x;
        end_y = rect->end_y;

        if (image->memory.ptrs[0] == NULL)
        {
            if (ownAllocateImage(image) == vx_false_e)
            {
                vxAddLogEntry((vx_reference)image, VX_ERROR_NO_MEMORY, "Failed to allocate image!\n");
                return 0;
            }
        }
        if (plane_index < image->planes)
        {
            vx_size numPixels = ((end_x-start_x)/image->scale[plane_index][VX_DIM_X]) *
                                ((end_y-start_y)/image->scale[plane_index][VX_DIM_Y]);
            vx_size pixelSize = image->memory.strides[plane_index][VX_DIM_X];
            ownPrintImage(image);
            VX_PRINT(VX_ZONE_IMAGE, "numPixels = "VX_FMT_SIZE" pixelSize = "VX_FMT_SIZE"\n", numPixels, pixelSize);
            size = numPixels * pixelSize;
        }
        else
        {
            vxAddLogEntry((vx_reference)image, VX_ERROR_INVALID_PARAMETERS, "Plane index %u is out of bounds!", plane_index);
        }

        VX_PRINT(VX_ZONE_IMAGE, "image %p for patch {%u,%u to %u,%u} has a byte size of "VX_FMT_SIZE"\n",
                 image, start_x, start_y, end_x, end_y, size);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Image Reference is invalid!\n");
    }
    return size;
}

VX_API_ENTRY vx_status VX_API_CALL vxAccessImagePatch(vx_image image,
                                    const vx_rectangle_t *rect,
                                    vx_uint32 plane_index,
                                    vx_imagepatch_addressing_t *addr,
                                    void **ptr,
                                    vx_enum usage)
{
    vx_uint8 *p = NULL;
    vx_status status = VX_FAILURE;
    vx_bool mapped = vx_false_e;
    vx_uint32 start_x = rect ? rect->start_x : 0u;
    vx_uint32 start_y = rect ? rect->start_y : 0u;
    vx_uint32 end_x = rect ? rect->end_x : 0u;
    vx_uint32 end_y = rect ? rect->end_y : 0u;
    vx_bool zero_area = ((((end_x - start_x) == 0) || ((end_y - start_y) == 0)) ? vx_true_e : vx_false_e);

    /* bad parameters */
    if ((usage < VX_READ_ONLY) || (VX_READ_AND_WRITE < usage) ||
        (addr == NULL) || (ptr == NULL))
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* bad references */
    if ((!rect) ||
        (ownIsValidImage(image) == vx_false_e))
    {
        status = VX_ERROR_INVALID_REFERENCE;
        goto exit;
    }

    /* determine if virtual before checking for memory */
    if (image->base.is_virtual == vx_true_e)
    {
        if (image->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" image. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual image\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            goto exit;
        }
        /* framework trying to access a virtual image, this is ok. */
    }

    /* more bad parameters */
    if (zero_area == vx_false_e &&
        ((plane_index >= image->memory.nptrs) ||
         (plane_index >= image->planes) ||
         (rect->start_x >= rect->end_x) ||
         (rect->start_y >= rect->end_y)))
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* The image needs to be allocated */
    if ((image->memory.ptrs[0] == NULL) && (ownAllocateImage(image) == vx_false_e))
    {
        VX_PRINT(VX_ZONE_ERROR, "No memory!\n");
        status = VX_ERROR_NO_MEMORY;
        goto exit;
    }

    /* can't write to constant */
    if ((image->constant == vx_true_e) && ((usage == VX_WRITE_ONLY) ||
                                           (usage == VX_READ_AND_WRITE)))
    {
        status = VX_ERROR_NOT_SUPPORTED;
        VX_PRINT(VX_ZONE_ERROR, "Can't write to constant data, only read!\n");
        vxAddLogEntry(&image->base, status, "Can't write to constant data, only read!\n");
        goto exit;
    }

    /*************************************************************************/
    VX_PRINT(VX_ZONE_IMAGE, "AccessImagePatch from "VX_FMT_REF" to ptr %p from {%u,%u} to {%u,%u} plane %u\n",
            image, *ptr, rect->start_x, rect->start_y, rect->end_x, rect->end_y, plane_index);

    /* POSSIBILITIES:
     * 1.) !*ptr && RO == MAP
     * 2.) !*ptr && WO == MAP
     * 3.) !*ptr && RW == MAP
     * 4.)  *ptr && RO||RW == COPY (UNLESS MAP)
     */

    if ((*ptr == NULL) && ((usage == VX_READ_ONLY) ||
                           (usage == VX_WRITE_ONLY) ||
                           (usage == VX_READ_AND_WRITE)))
    {
        mapped = vx_true_e;
    }

    /* MAP mode */
    if (mapped == vx_true_e)
    {
        vx_uint32 index = 0u;

        /* lock the memory plane for multiple writers*/
        if (usage != VX_READ_ONLY) {
            if (ownSemWait(&image->memory.locks[plane_index]) == vx_false_e) {
                status = VX_ERROR_NO_RESOURCES;
                goto exit;
            }
        }
        ownPrintMemory(&image->memory);
        p = (vx_uint8 *)image->memory.ptrs[plane_index];

        /* use the addressing of the internal format */
        addr->dim_x = rect->end_x - rect->start_x;
        addr->dim_y = rect->end_y - rect->start_y;
        addr->stride_x = image->memory.strides[plane_index][VX_DIM_X];
        addr->stride_y = image->memory.strides[plane_index][VX_DIM_Y];
        addr->step_x = image->scale[plane_index][VX_DIM_X];
        addr->step_y = image->scale[plane_index][VX_DIM_Y];
        addr->scale_x = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_X];
        addr->scale_y = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_Y];

        index = vxComputePatchOffset(rect->start_x, rect->start_y, addr);
        *ptr = &p[index];
        VX_PRINT(VX_ZONE_IMAGE, "Returning mapped pointer %p which is offset by %lu\n", *ptr, index);

        ownReadFromReference(&image->base);
        ownIncrementReference(&image->base, VX_EXTERNAL);

        status = VX_SUCCESS;
    }

    /* COPY mode */
    else
    {
        vx_size size = vxComputeImagePatchSize(image, rect, plane_index);
        vx_uint32 a = 0u;

        vx_imagepatch_addressing_t *addr_save = calloc(1, sizeof(vx_imagepatch_addressing_t));
        /* Strides given by the application */
        addr_save->stride_x = addr->stride_x;
        addr_save->stride_y = addr->stride_y;
        /* Computed by the application */
        addr->dim_x = addr_save->dim_x = rect->end_x - rect->start_x;
        addr->dim_y = addr_save->dim_y = rect->end_y - rect->start_y;
        addr->step_x = addr_save->step_x = image->scale[plane_index][VX_DIM_X];
        addr->step_y = addr_save->step_y = image->scale[plane_index][VX_DIM_Y];
        addr->scale_x = addr_save->scale_x = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_X];
        addr->scale_y = addr_save->scale_y = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_Y];

        if (ownAddAccessor(image->base.context, size, usage, *ptr, &image->base, &a, addr_save) == vx_false_e)
        {
            status = VX_ERROR_NO_MEMORY;
            vxAddLogEntry(&image->base, status, "Failed to allocate memory for COPY! Size="VX_FMT_SIZE"\n", size);
            goto exit;
        }

        {
            vx_uint32 x, y;
            vx_uint8 *tmp = *ptr;

            /*! \todo implement overlapping multi-writers lock, not just single writer lock */
            if ((usage == VX_WRITE_ONLY) || (usage == VX_READ_AND_WRITE))
            {
                if (ownSemWait(&image->memory.locks[plane_index]) == vx_false_e)
                {
                    status = VX_ERROR_NO_RESOURCES;
                    goto exit;
                }
            }

            if ((usage == VX_READ_ONLY) || (usage == VX_READ_AND_WRITE))
            {
                if (addr_save->stride_x == image->memory.strides[plane_index][VX_DIM_X])
                {
                    /* Both have compact lines */
                    for (y = rect->start_y; y < rect->end_y; y+=addr_save->step_y)
                    {
                        vx_uint32 i = vxComputePlaneOffset(image, rect->start_x, y, plane_index);
                        vx_uint32 j = vxComputePatchOffset(0, (y - rect->start_y), addr);
                        vx_uint32 len = vxComputePlaneRangeSize(image, addr_save->dim_x, plane_index);
                        VX_PRINT(VX_ZONE_IMAGE, "%p[%u] <= %p[%u] for %u\n", tmp, j, image->memory.ptrs[plane_index], i, len);
                        memcpy(&tmp[j], &image->memory.ptrs[plane_index][i], len);
                    }
                }

                else
                {
                    /* The destination is not compact, we need to copy per element */
                    vx_uint8 *pDestLine = &tmp[0];
                    for (y = rect->start_y; y < rect->end_y; y+=addr_save->step_y)
                    {
                        vx_uint8 *pDest = pDestLine;

                        vx_uint32 offset = vxComputePlaneOffset(image, rect->start_x, y, plane_index);
                        vx_uint8 *pSrc = &image->memory.ptrs[plane_index][offset];

                        for (x = rect->start_x; x < rect->end_x; x+=addr_save->step_x)
                        {
                            /* One element */
                            memcpy(pDest, pSrc, image->memory.strides[plane_index][VX_DIM_X]);

                            pSrc += image->memory.strides[plane_index][VX_DIM_X];
                            pDest += addr_save->stride_x;
                        }

                        pDestLine += addr_save->stride_y;
                    }
                }

                VX_PRINT(VX_ZONE_IMAGE, "Copied image into %p\n", *ptr);
                ownReadFromReference(&image->base);
            }

            ownIncrementReference(&image->base, VX_EXTERNAL);

            status = VX_SUCCESS;
        }
    }
exit:
    VX_PRINT(VX_ZONE_API, "returned %d\n", status);
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxCommitImagePatch(vx_image image,
                                    const vx_rectangle_t *rect,
                                    vx_uint32 plane_index,
                                    const vx_imagepatch_addressing_t *addr,
                                    const void *ptr)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    vx_int32 i = 0;
    vx_bool external = vx_true_e; // assume that it was an allocated buffer
    vx_uint32 start_x = rect ? rect->start_x : 0u;
    vx_uint32 start_y = rect ? rect->start_y : 0u;
    vx_uint32 end_x = rect ? rect->end_x : 0u;
    vx_uint32 end_y = rect ? rect->end_y : 0u;
    vx_uint8 *tmp = (vx_uint8 *)ptr;
    vx_bool zero_area = ((((end_x - start_x) == 0) || ((end_y - start_y) == 0)) ? vx_true_e : vx_false_e);
    vx_uint32 index = UINT32_MAX; // out of bounds, if given to remove, won't do anything

    if (ownIsValidImage(image) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    VX_PRINT(VX_ZONE_IMAGE, "CommitImagePatch to "VX_FMT_REF" from ptr %p plane %u to {%u,%u},{%u,%u}\n",
        image, ptr, plane_index, start_x, start_y, end_x, end_y);

    ownPrintImage(image);
    ownPrintImageAddressing(addr);

    /* determine if virtual before checking for memory */
    if (image->base.is_virtual == vx_true_e && zero_area == vx_false_e)
    {
        if (image->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" image. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual image\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            goto exit;
        }
        /* framework trying to access a virtual image, this is ok. */
    }

    if (zero_area == vx_false_e &&
        ((plane_index >= image->planes) ||
         (plane_index >= image->memory.nptrs) ||
         (ptr == NULL) ||
         (addr == NULL)))
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    /* check the rectangle, it has to be in actual plane space */
    if (zero_area == vx_false_e &&
        ((start_x >= end_x) || ((end_x - start_x) > addr->dim_x) ||
        (start_y >= end_y) || ((end_y - start_y) > addr->dim_y) ||
        (end_x > (vx_uint32)image->memory.dims[plane_index][VX_DIM_X] * (vx_uint32)image->scale[plane_index][VX_DIM_X]) ||
        (end_y > (vx_uint32)image->memory.dims[plane_index][VX_DIM_Y] * (vx_uint32)image->scale[plane_index][VX_DIM_X])))
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid start,end coordinates! plane %u {%u,%u},{%u,%u}\n",
                 plane_index, start_x, start_y, end_x, end_y);
        ownPrintImage(image);
        DEBUG_BREAK();
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    {
        /* VARIABLES:
         * 1.) ZERO_AREA
         * 2.) CONSTANT - independant
         * 3.) INTERNAL - independant of area
         * 4.) EXTERNAL - dependant on area (do nothing on zero, determine on non-zero)
         * 5.) !INTERNAL && !EXTERNAL == MAPPED
         */
        vx_bool internal = ownFindAccessor(image->base.context, ptr, &index);

        if ((zero_area == vx_false_e) && (image->constant == vx_true_e))
        {
            /* we tried to modify constant data! */
            VX_PRINT(VX_ZONE_ERROR, "Can't set constant image data!\n");
            status = VX_ERROR_NOT_SUPPORTED;
            /* don't modify the accessor here, it's an error case */
            goto exit;
        }
        else if (zero_area == vx_false_e && image->constant == vx_false_e)
        {
            /* this could be a write-back */
            if (internal == vx_true_e && image->base.context->accessors[index].usage == VX_READ_ONLY)
            {
                /* this is a buffer that we allocated on behalf of the user and now they are done. Do nothing else*/
                ownRemoveAccessor(image->base.context, index);
            }
            else
            {
                /* determine if this grows the valid region */
                if (image->region.start_x > start_x)
                    image->region.start_x = start_x;
                if (image->region.start_y > start_y)
                    image->region.start_y = start_y;
                if (image->region.end_x < end_x)
                    image->region.end_x = end_x;
                if (image->region.end_y < end_y)
                    image->region.end_y = end_y;

                /* index of 1 pixel line past last. */
                i = (image->memory.dims[plane_index][VX_DIM_Y] * image->memory.strides[plane_index][VX_DIM_Y]);

                VX_PRINT(VX_ZONE_IMAGE, "base:%p tmp:%p end:%p\n",
                    image->memory.ptrs[plane_index], ptr, &image->memory.ptrs[plane_index][i]);

                if ((image->memory.ptrs[plane_index] <= (vx_uint8 *)ptr) &&
                    ((vx_uint8 *)ptr < &image->memory.ptrs[plane_index][i]))
                {
                    /* corner case for 2d memory */
                    if (image->memory.strides[plane_index][VX_DIM_Y] !=
                        ((int)image->memory.dims[plane_index][VX_DIM_X] * image->memory.strides[plane_index][VX_DIM_X]))
                    {
                        /* determine if the pointer is within the image boundary. */
                        vx_uint8 *base = image->memory.ptrs[plane_index];
                        vx_size offset = ((vx_size)(tmp - base)) % image->memory.strides[plane_index][VX_DIM_Y];
                        if (offset < (vx_size)(image->memory.dims[plane_index][VX_DIM_X] * image->memory.strides[plane_index][VX_DIM_X]))
                        {
                            VX_PRINT(VX_ZONE_IMAGE, "Pointer is within 2D image\n");
                            external = vx_false_e;
                        }
                    }
                    else
                    {
                        /* the pointer in contained in the image, so it was mapped, thus
                         * there's nothing else to do. */
                        external = vx_false_e;
                        VX_PRINT(VX_ZONE_IMAGE, "Mapped pointer detected!\n");
                    }
                }
                if (external == vx_true_e || internal == vx_true_e)
                {
                    if (internal == vx_true_e)
                    {
                        /* copy the patch back to the image. */
                        if (addr->stride_x == image->memory.strides[plane_index][VX_DIM_X])
                        {
                            /* Both source and destination have compact lines */
                            vx_uint32 y;
                            for (y = start_y; y < end_y; y+=addr->step_y)
                            {
                                vx_uint32 i = vxComputePlaneOffset(image, start_x, y, plane_index);
                                vx_uint32 j = vxComputePatchOffset(0, (y - start_y), addr);
                                vx_uint32 len = vxComputePatchRangeSize((end_x - start_x), addr);
                                VX_PRINT(VX_ZONE_IMAGE, "%p[%u] <= %p[%u] for %u\n", image->memory.ptrs[plane_index], j, tmp, i, len);
                                memcpy(&image->memory.ptrs[plane_index][i], &tmp[j], len);
                            }
                        }

                        else
                        {
                            /* The source is not compact, we need to copy per element */
                            vx_uint32 x, y;
                            vx_uint8 *pDestLine = &tmp[0];
                            for (y = start_y; y < end_y; y+=addr->step_y)
                            {
                                vx_uint8 *pSrc = pDestLine;

                                vx_uint32 offset = vxComputePlaneOffset(image, start_x, y, plane_index);
                                vx_uint8 *pDest = &image->memory.ptrs[plane_index][offset];

                                for (x = start_x; x < end_x; x+=addr->step_x)
                                {
                                    /* One element */
                                    memcpy(pDest, pSrc, image->memory.strides[plane_index][VX_DIM_X]);

                                    pDest += image->memory.strides[plane_index][VX_DIM_X];
                                    pSrc += addr->stride_x;
                                }

                                pDestLine += addr->stride_y;
                            }
                        }

                        /* a write only or read/write copy */
                        ownRemoveAccessor(image->base.context, index);
                    }
                    else
                    {
                        /* copy the patch back to the image. */
                        vx_uint32 y, i, j, len;
                        for (y = start_y; y < end_y; y += addr->step_y)
                        {
                            i = vxComputePlaneOffset(image, start_x, y, plane_index);
                            j = vxComputePatchOffset(0, (y - start_y), addr);
                            len = vxComputePatchRangeSize((end_x - start_x), addr);
                            VX_PRINT(VX_ZONE_IMAGE, "%p[%u] <= %p[%u] for %u\n", image->memory.ptrs[plane_index], j, tmp, i, len);
                            memcpy(&image->memory.ptrs[plane_index][i], &tmp[j], len);
                        }

                    }
                }
                ownWroteToReference(&image->base);
            }
            status = VX_SUCCESS;
            ownSemPost(&image->memory.locks[plane_index]);
        }
        else if (zero_area == vx_true_e)
        {
            /* could be RO|WO|RW where they decided not to commit anything. */
            if (internal == vx_true_e) // RO
            {
                ownRemoveAccessor(image->base.context, index);
            }
            else // RW|WO
            {
                /*! \bug (possible bug, but maybe not) anyone can decrement an
                 *  image access, should we limit to incrementor? that would be
                 *  a lot to track */
                ownSemPost(&image->memory.locks[plane_index]);
            }
            status = VX_SUCCESS;
        }
        VX_PRINT(VX_ZONE_IMAGE, "Decrementing Image Reference\n");
        ownDecrementReference(&image->base, VX_EXTERNAL);
    }
exit:
    VX_PRINT(VX_ZONE_API, "return %d\n", status);
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxCopyImagePatch(
    vx_image image,
    const vx_rectangle_t* rect,
    vx_uint32 plane_index,
    const vx_imagepatch_addressing_t* addr,
    void* ptr,
    vx_enum usage,
    vx_enum mem_type)
{
    vx_status status = VX_FAILURE;
    (void)mem_type;

    vx_uint32 start_x = rect ? rect->start_x : 0u;
    vx_uint32 start_y = rect ? rect->start_y : 0u;
    vx_uint32 end_x = rect ? rect->end_x : 0u;
    vx_uint32 end_y = rect ? rect->end_y : 0u;
    vx_bool zero_area = ((((end_x - start_x) == 0) || ((end_y - start_y) == 0)) ? vx_true_e : vx_false_e);

    /* bad parameters */
    if ( ((VX_READ_ONLY != usage) && (VX_WRITE_ONLY != usage)) ||
         (rect == NULL) || (addr == NULL) || (ptr == NULL) )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* bad references */
    if ( ownIsValidImage(image) == vx_false_e )
    {
        status = VX_ERROR_INVALID_REFERENCE;
        goto exit;
    }

    /* determine if virtual before checking for memory */
    if (image->base.is_virtual == vx_true_e)
    {
        if (image->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" image. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual image\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            goto exit;
        }
        /* framework trying to access a virtual image, this is ok. */
    }

    /* more bad parameters */
    if (zero_area == vx_false_e &&
        ((plane_index >= image->memory.nptrs) ||
         (plane_index >= image->planes) ||
         (start_x >= end_x) ||
         (start_y >= end_y)))
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* The image needs to be allocated */
    if ((image->memory.ptrs[0] == NULL) && (ownAllocateImage(image) == vx_false_e))
    {
        VX_PRINT(VX_ZONE_ERROR, "No memory!\n");
        status = VX_ERROR_NO_MEMORY;
        goto exit;
    }

    /* can't write to constant */
    if ( (image->constant == vx_true_e) && (usage == VX_WRITE_ONLY) )
    {
        status = VX_ERROR_NOT_SUPPORTED;
        VX_PRINT(VX_ZONE_ERROR, "Can't write to constant data, only read!\n");
        vxAddLogEntry(&image->base, status, "Can't write to constant data, only read!\n");
        goto exit;
    }

    /*************************************************************************/
    VX_PRINT(VX_ZONE_IMAGE, "CopyImagePatch from "VX_FMT_REF" to ptr %p from {%u,%u} to {%u,%u} plane %u\n",
        image, ptr, start_x, start_y, end_x, end_y, plane_index);

    if (usage == VX_READ_ONLY)
    {
        /* Copy from image (READ) mode */

        vx_uint32 x;
        vx_uint32 y;
        vx_uint8* pSrc = image->memory.ptrs[plane_index];
        vx_uint8* pDst = ptr;

        vx_imagepatch_addressing_t addr_save = VX_IMAGEPATCH_ADDR_INIT;

        /* Strides given by the application */
        addr_save.dim_x    = addr->dim_x;
        addr_save.dim_y    = addr->dim_y;
        addr_save.stride_x = addr->stride_x;
        addr_save.stride_y = addr->stride_y;

        addr_save.step_x  = image->scale[plane_index][VX_DIM_X];
        addr_save.step_y  = image->scale[plane_index][VX_DIM_Y];
        addr_save.scale_x = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_X];
        addr_save.scale_y = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_Y];

        /* copy the patch from the image */
        if (addr_save.stride_x == image->memory.strides[plane_index][VX_DIM_X])
        {
            /* Both have compact lines */
            for (y = start_y; y < end_y; y += addr_save.step_y)
            {
                vx_uint32 srcOffset = vxComputePlaneOffset(image, start_x, y, plane_index);
                vx_uint8* pSrcLine = &pSrc[srcOffset];

                vx_uint32 dstOffset = vxComputePatchOffset(0, (y - start_y), &addr_save);
                vx_uint8* pDstLine = &pDst[dstOffset];

                vx_uint32 len = vxComputePlaneRangeSize(image, end_x - start_x/*image->width*/, plane_index);

                VX_PRINT(VX_ZONE_IMAGE, "%p[%u] <= %p[%u] for %u\n", pDst, dstOffset, pSrc, srcOffset, len);

                memcpy(pDstLine, pSrcLine, len);
            }
        }
        else
        {
            /* The destination is not compact, we need to copy per element */
            for (y = start_y; y < end_y; y += addr_save.step_y)
            {
                vx_uint32 srcOffset = vxComputePlaneOffset(image, start_x, y, plane_index);
                vx_uint8* pSrcLine = &pSrc[srcOffset];

                vx_uint8* pDstLine = pDst;

                vx_uint32 len = image->memory.strides[plane_index][VX_DIM_X];

                for (x = start_x; x < end_x; x += addr_save.step_x)
                {
                    /* One element */
                    memcpy(pDstLine, pSrcLine, len);

                    pSrcLine += len;
                    pDstLine += addr_save.stride_x;
                }

                pDst += addr_save.stride_y;
            }
        }

        VX_PRINT(VX_ZONE_IMAGE, "Copied image into %p\n", ptr);

        ownReadFromReference(&image->base);
    }
    else
    {
        /* Copy to image (WRITE) mode */
        vx_uint32 x;
        vx_uint32 y;
        vx_uint8* pSrc = ptr;
        vx_uint8* pDst = image->memory.ptrs[plane_index];

        vx_imagepatch_addressing_t addr_save = VX_IMAGEPATCH_ADDR_INIT;

        /* Strides given by the application */
        addr_save.dim_x    = addr->dim_x;
        addr_save.dim_y    = addr->dim_y;
        addr_save.stride_x = addr->stride_x;
        addr_save.stride_y = addr->stride_y;

        addr_save.step_x  = image->scale[plane_index][VX_DIM_X];
        addr_save.step_y  = image->scale[plane_index][VX_DIM_Y];
        addr_save.scale_x = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_X];
        addr_save.scale_y = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_Y];

        /* lock image plane from multiple writers */
        if (ownSemWait(&image->memory.locks[plane_index]) == vx_false_e)
        {
            status = VX_ERROR_NO_RESOURCES;
            goto exit;
        }

        /* copy the patch to the image */
        if (addr_save.stride_x == image->memory.strides[plane_index][VX_DIM_X])
        {
            /* Both source and destination have compact lines */
            for (y = start_y; y < end_y; y += addr_save.step_y)
            {
                vx_uint32 srcOffset = vxComputePatchOffset(0, (y - start_y), &addr_save);
                vx_uint8* pSrcLine = &pSrc[srcOffset];

                vx_uint32 dstOffset = vxComputePlaneOffset(image, start_x, y, plane_index);
                vx_uint8* pDstLine = &pDst[dstOffset];

                vx_uint32 len = vxComputePatchRangeSize((end_x - start_x), &addr_save);

                VX_PRINT(VX_ZONE_IMAGE, "%p[%u] <= %p[%u] for %u\n", pDst, srcOffset, pSrc, dstOffset, len);

                memcpy(pDstLine, pSrcLine, len);
            }
        }
        else
        {
            /* The destination is not compact, we need to copy per element */
            for (y = start_y; y < end_y; y += addr_save.step_y)
            {
                vx_uint8* pSrcLine = pSrc;

                vx_uint32 dstOffset = vxComputePlaneOffset(image, start_x, y, plane_index);
                vx_uint8* pDstLine = &pDst[dstOffset];

                vx_uint32 len = image->memory.strides[plane_index][VX_DIM_X];

                for (x = start_x; x < end_x; x += addr_save.step_x)
                {
                    /* One element */
                    memcpy(pDstLine, pSrcLine, len);

//                    pSrcLine += len;
//                    pDstLine += addr_save.stride_x;
                    pSrcLine += addr_save.stride_x;
                    pDstLine += len;
                }

                pSrc += addr_save.stride_y;
            }
        }

        VX_PRINT(VX_ZONE_IMAGE, "Copied to image from %p\n", ptr);

        ownWroteToReference(&image->base);
        /* unlock image plane */
        ownSemPost(&image->memory.locks[plane_index]);
    }

    status = VX_SUCCESS;

exit:

    VX_PRINT(VX_ZONE_API, "returned %d\n", status);

    return status;
} /* vxCopyImagePatch() */


VX_API_ENTRY vx_status VX_API_CALL vxMapImagePatch(
    vx_image image,
    const vx_rectangle_t* rect,
    vx_uint32 plane_index,
    vx_map_id* map_id,
    vx_imagepatch_addressing_t* addr,
    void** ptr,
    vx_enum usage,
    vx_enum mem_type,
    vx_uint32 flags)
{
    vx_uint32 start_x = rect ? rect->start_x : 0u;
    vx_uint32 start_y = rect ? rect->start_y : 0u;
    vx_uint32 end_x   = rect ? rect->end_x : 0u;
    vx_uint32 end_y   = rect ? rect->end_y : 0u;
    vx_bool zero_area = ((((end_x - start_x) == 0) || ((end_y - start_y) == 0)) ? vx_true_e : vx_false_e);
    vx_status status = VX_FAILURE;

    /* bad parameters */
    if ( (rect == NULL) || (map_id == NULL) || (addr == NULL) || (ptr == NULL) )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* bad references */
    if (ownIsValidImage(image) == vx_false_e)
    {
        status = VX_ERROR_INVALID_REFERENCE;
        goto exit;
    }

    /* determine if virtual before checking for memory */
    if (image->base.is_virtual == vx_true_e)
    {
        if (image->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" image. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual image\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            goto exit;
        }
        /* framework trying to access a virtual image, this is ok. */
    }

    /* more bad parameters */
    if (zero_area == vx_false_e &&
        ((plane_index >= image->memory.nptrs) ||
        (plane_index >= image->planes) ||
        (start_x >= end_x) ||
        (start_y >= end_y)))
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* The image needs to be allocated */
    if ((image->memory.ptrs[0] == NULL) && (ownAllocateImage(image) == vx_false_e))
    {
        VX_PRINT(VX_ZONE_ERROR, "No memory!\n");
        status = VX_ERROR_NO_MEMORY;
        goto exit;
    }

    /* can't write to constant */
    if ((image->constant == vx_true_e) &&
        ((usage == VX_WRITE_ONLY) || (usage == VX_READ_AND_WRITE)))
    {
        status = VX_ERROR_NOT_SUPPORTED;
        VX_PRINT(VX_ZONE_ERROR, "Can't write to constant data, only read!\n");
        vxAddLogEntry(&image->base, status, "Can't write to constant data, only read!\n");
        goto exit;
    }

    /*************************************************************************/
    VX_PRINT(VX_ZONE_IMAGE, "MapImagePatch from "VX_FMT_REF" to ptr %p from {%u,%u} to {%u,%u} plane %u\n",
        image, *ptr, start_x, start_y, end_x, end_y, plane_index);

    /* MAP mode */
    {
        vx_size size;
        vx_memory_map_extra extra;
        vx_uint8* buf = 0;

        size = vxComputeImagePatchSize(image, rect, plane_index);

        extra.image_data.plane_index = plane_index;
        extra.image_data.rect        = *rect;

        if (VX_MEMORY_TYPE_HOST == image->memory_type && vx_true_e == ownMemoryMap(image->base.context, (vx_reference)image, 0, usage, mem_type, flags, &extra, (void**)&buf, map_id))
        {
            /* use the addressing of the internal format */
            addr->dim_x    = end_x - start_x;
            addr->dim_y    = end_y - start_y;
            addr->stride_x = image->memory.strides[plane_index][VX_DIM_X];
            addr->stride_y = image->memory.strides[plane_index][VX_DIM_Y];
            addr->step_x   = image->scale[plane_index][VX_DIM_X];
            addr->step_y   = image->scale[plane_index][VX_DIM_Y];
            addr->scale_x  = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_X];
            addr->scale_y  = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_Y];

            buf = image->memory.ptrs[plane_index];

            vx_uint32 index = vxComputePatchOffset(rect->start_x, rect->start_y, addr);
            *ptr = &buf[index];
            VX_PRINT(VX_ZONE_IMAGE, "Returning mapped pointer %p\n", *ptr);

            ownIncrementReference(&image->base, VX_EXTERNAL);

            status = VX_SUCCESS;
        }
        else
        /* get mapping buffer of sufficient size and map_id */
        if (vx_true_e == ownMemoryMap(image->base.context, (vx_reference)image, size, usage, mem_type, flags, &extra, (void**)&buf, map_id))
        {
            /* use the addressing of the internal format */
            addr->dim_x    = end_x - start_x;
            addr->dim_y    = end_y - start_y;
            addr->stride_x = image->memory.strides[plane_index][VX_DIM_X];
            addr->stride_y = addr->stride_x * addr->dim_x / image->scale[plane_index][VX_DIM_Y];
            addr->step_x   = image->scale[plane_index][VX_DIM_X];
            addr->step_y   = image->scale[plane_index][VX_DIM_Y];
            addr->scale_x  = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_X];
            addr->scale_y  = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_Y];

            /* initialize mapping buffer with image patch data for read/read-modify-write access */
            if (VX_READ_ONLY == usage || VX_READ_AND_WRITE == usage)
            {
                /* lock image plane even for read access to avoid mix of simultaneous read/write */
                if (vx_true_e == ownSemWait(&image->memory.locks[plane_index]))
                {
                    vx_uint32 y;
                    vx_uint8* pSrc = image->memory.ptrs[plane_index];
                    vx_uint8* pDst = buf;

                    ownPrintMemory(&image->memory);

                    /* Both have compact lines */
                    for (y = start_y; y < end_y; y += addr->step_y)
                    {
                        vx_uint32 srcOffset = vxComputePlaneOffset(image, start_x, y, plane_index);
                        vx_uint8* pSrcLine = &pSrc[srcOffset];

                        vx_uint32 dstOffset = vxComputePatchOffset(0, (y - start_y), addr);
                        vx_uint8* pDstLine = &pDst[dstOffset];

                        vx_uint32 len = vxComputePlaneRangeSize(image, addr->dim_x, plane_index);

                        VX_PRINT(VX_ZONE_IMAGE, "%p[%u] <= %p[%u] for %u\n", pDst, dstOffset, pSrc, srcOffset, len);

                        memcpy(pDstLine, pSrcLine, len);
                    }

                    ownReadFromReference(&image->base);

                    /* we're done, unlock the image plane */
                    ownSemPost(&image->memory.locks[plane_index]);
                }
                else
                {
                    status = VX_FAILURE;
                    VX_PRINT(VX_ZONE_ERROR, "Can't lock memory plane for mapping\n");
                    goto exit;
                }
            } /* if VX_READ_ONLY or VX_READ_AND_WRITE */

            *ptr = buf;
            VX_PRINT(VX_ZONE_IMAGE, "Returning mapped pointer %p\n", *ptr);

            ownIncrementReference(&image->base, VX_EXTERNAL);

            status = VX_SUCCESS;
        }
        else
            status = VX_FAILURE;
    }

exit:
    VX_PRINT(VX_ZONE_API, "return %d\n", status);

    return status;
} /* vxMapImagePatch() */

VX_API_ENTRY vx_status VX_API_CALL vxUnmapImagePatch(vx_image image, vx_map_id map_id)
{
    vx_status status = VX_FAILURE;

    /* bad references */
    if (ownIsValidImage(image) == vx_false_e)
    {
        status = VX_ERROR_INVALID_REFERENCE;
        goto exit;
    }

    /* bad parameters */
    if (ownFindMemoryMap(image->base.context, (vx_reference)image, map_id) != vx_true_e)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        VX_PRINT(VX_ZONE_ERROR, "Invalid parameters to unmap image patch\n");
        return status;
    }

    {
        vx_context       context = image->base.context;
        vx_memory_map_t* map     = &context->memory_maps[map_id];

        if (map->used &&
            map->ref == (vx_reference)image)
        {
            /* commit changes for write access */
            if ((VX_WRITE_ONLY == map->usage || VX_READ_AND_WRITE == map->usage) && NULL != map->ptr)
            {
                vx_uint32      plane_index = map->extra.image_data.plane_index;
                vx_rectangle_t rect        = map->extra.image_data.rect;

                /* lock image plane for simultaneous write */
                if (vx_true_e == ownSemWait(&image->memory.locks[plane_index]))
                {
                    vx_uint32 y;
                    vx_uint8* pSrc = map->ptr;
                    vx_uint8* pDst = image->memory.ptrs[plane_index];
                    vx_imagepatch_addressing_t addr = VX_IMAGEPATCH_ADDR_INIT;

                    /* use the addressing of the internal format */
                    addr.dim_x    = rect.end_x - rect.start_x;
                    addr.dim_y    = rect.end_y - rect.start_y;
                    addr.stride_x = image->memory.strides[plane_index][VX_DIM_X];
                    addr.stride_y = addr.stride_x * addr.dim_x / image->scale[plane_index][VX_DIM_Y];
                    addr.step_x   = image->scale[plane_index][VX_DIM_X];
                    addr.step_y   = image->scale[plane_index][VX_DIM_Y];
                    addr.scale_x  = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_X];
                    addr.scale_y  = VX_SCALE_UNITY / image->scale[plane_index][VX_DIM_Y];

                    /* Both source and destination have compact lines */
                    for (y = rect.start_y; y < rect.end_y; y += addr.step_y)
                    {
                        vx_uint32 srcOffset = vxComputePatchOffset(0, (y - rect.start_y), &addr);
                        vx_uint8* pSrcLine = &pSrc[srcOffset];

                        vx_uint32 dstOffset = vxComputePlaneOffset(image, rect.start_x, y, plane_index);
                        vx_uint8* pDstLine = &pDst[dstOffset];

                        vx_uint32 len = vxComputePatchRangeSize((rect.end_x - rect.start_x), &addr);

                        VX_PRINT(VX_ZONE_IMAGE, "%p[%u] <= %p[%u] for %u\n", pDst, srcOffset, pSrc, dstOffset, len);

                        memcpy(pDstLine, pSrcLine, len);
                    }

                    /* we're done, unlock the image plane */
                    ownSemPost(&image->memory.locks[plane_index]);
                }
                else
                {
                    status = VX_FAILURE;
                    VX_PRINT(VX_ZONE_ERROR, "Can't lock memory plane for unmapping\n");
                    goto exit;
                }
            }

            /* freeing mapping buffer */
            ownMemoryUnmap(context, (vx_uint32)map_id);

            ownDecrementReference(&image->base, VX_EXTERNAL);

            status = VX_SUCCESS;
        }
        else
            status = VX_FAILURE;
    }

exit:
    VX_PRINT(VX_ZONE_API, "return %d\n", status);

    return status;
} /* vxUnmapImagePatch() */

VX_API_ENTRY void* VX_API_CALL vxFormatImagePatchAddress1d(void *ptr, vx_uint32 index, const vx_imagepatch_addressing_t *addr)
{
    vx_uint8 *new_ptr = NULL;
    if (ptr && index < addr->dim_x*addr->dim_y)
    {
        vx_uint32 x = index % addr->dim_x;
        vx_uint32 y = index / addr->dim_x;
        vx_uint32 offset = vxComputePatchOffset(x, y, addr);
        new_ptr = (vx_uint8 *)ptr;
        new_ptr = &new_ptr[offset];
    }
    return new_ptr;
}

VX_API_ENTRY void* VX_API_CALL vxFormatImagePatchAddress2d(void *ptr, vx_uint32 x, vx_uint32 y, const vx_imagepatch_addressing_t *addr)
{
    vx_uint8 *new_ptr = NULL;
    if (ptr && x < addr->dim_x && y < addr->dim_y)
    {
        vx_uint32 offset = vxComputePatchOffset(x, y, addr);
        new_ptr = (vx_uint8 *)ptr;
        new_ptr = &new_ptr[offset];
    }
    return new_ptr;
}

VX_API_ENTRY vx_status VX_API_CALL vxGetValidRegionImage(vx_image image, vx_rectangle_t *rect)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (ownIsValidImage(image) == vx_true_e)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        if (rect)
        {
            if ((image->region.start_x <= image->region.end_x) && (image->region.start_y <= image->region.end_y))
            {
                rect->start_x = image->region.start_x;
                rect->start_y = image->region.start_y;
                rect->end_x = image->region.end_x;
                rect->end_y = image->region.end_y;
            }
            else
            {
                rect->start_x = 0;
                rect->start_y = 0;
                rect->end_x = image->width;
                rect->end_y = image->height;
            }
            status = VX_SUCCESS;
        }
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxSetImageValidRectangle(vx_image image, const vx_rectangle_t* rect)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;

    if (ownIsValidImage(image) == vx_true_e)
    {
        if (rect)
        {
            if ((rect->start_x <= rect->end_x) && (rect->start_y <= rect->end_y) &&
                (rect->end_x <= image->width) && (rect->end_y <= image->height))
            {
                image->region.start_x = rect->start_x;
                image->region.start_y = rect->start_y;
                image->region.end_x   = rect->end_x;
                image->region.end_y   = rect->end_y;
                status = VX_SUCCESS;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
        }
        else
        {
            image->region.start_x = 0;
            image->region.start_y = 0;
            image->region.end_x   = image->width;
            image->region.end_y   = image->height;
            status = VX_SUCCESS;
        }
    }

    return status;
}

void ownPrintImage(vx_image image)
{
    vx_uint32 p = 0;
    vx_char df_image[5];
    strncpy(df_image, (char *)&image->format, 4);
    df_image[4] = '\0';
    ownPrintReference(&image->base);
    VX_PRINT(VX_ZONE_IMAGE, "vx_image_t:%p %s %ux%u (%s)\n", image, df_image, image->width, image->height, (image->constant?"CONSTANT":"MUTABLE"));
    for (p = 0; p < image->planes; p++)
    {
        VX_PRINT(VX_ZONE_IMAGE, "\tplane[%u] ptr:%p dim={%u,%u,%u} stride={%d,%d,%d} scale={%u,%u,%u} bounds={%u,%ux%u,%u}\n",
                     p,
                     image->memory.ptrs[p],
                     image->memory.dims[p][VX_DIM_C],
                     image->memory.dims[p][VX_DIM_X],
                     image->memory.dims[p][VX_DIM_Y],
                     image->memory.strides[p][VX_DIM_C],
                     image->memory.strides[p][VX_DIM_X],
                     image->memory.strides[p][VX_DIM_Y],
                     image->scale[p][VX_DIM_C],
                     image->scale[p][VX_DIM_X],
                     image->scale[p][VX_DIM_Y],
                     image->bounds[p][VX_DIM_X][VX_BOUND_START],
                     image->bounds[p][VX_DIM_X][VX_BOUND_END],
                     image->bounds[p][VX_DIM_Y][VX_BOUND_START],
                     image->bounds[p][VX_DIM_Y][VX_BOUND_END]);
    }
}
