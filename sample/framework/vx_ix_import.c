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
#ifdef OPENVX_USE_IX

#include <vx_internal.h>
#include <VX/vx_khr_nn.h>
#ifndef VX_IX_USE_IMPORT_AS_KERNEL
#define VX_IX_USE_IMPORT_AS_KERNEL (VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_IX_USE) + 0x3) /*!< \brief Graph exported as user kernel. */
#endif
VX_API_ENTRY vx_enum VX_API_CALL vxGetUserStructByName(vx_context context, const vx_char *name);

/*!
 * \file
 * \brief The Import Object from Binary API.
 *  See vx_ix_format.txt for details of the export format
 */

/*
Import objects from binary (export is in a separate file vx_ix_export.c)


This is for VX 1.2.0, and the format revision is 1

*IMPORTANT NOTE*
================
This implementation requires that the exporting and importing implementations have the same endianness.
This implementation does not support exports originating from previous versions!

*/
#define DEBUGPRINTF(x,...)
//#define DEBUGPRINTF printf

/* User kernel that calls a graph */
static vx_status graph_kernel_validate(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_SUCCESS;
    /* Detect recursion using the kernel local data size TODO - is this safe during pipelining? */
    DEBUGPRINTF("Graph kernel validation\n");
    if (node->kernel->attributes.localDataSize)
    {
        DEBUGPRINTF("Recursion detected\n");
        status = VX_FAILURE;
    }
    else
    {
        vx_graph graph = (vx_graph)node->kernel->base.scope;    /* get the graph */
        vx_uint32 p;                                            /* for parameter enumeration */
        node->kernel->attributes.localDataSize = 1;             /* For recursion detection */
        /* Set all parameters to graph */
        status = vxGetStatus(node->kernel->base.scope);
        if (status)
        {
            DEBUGPRINTF("Graph is not valid: status = %d\n", status);
        }
        else if (VX_TYPE_GRAPH != node->kernel->base.scope->type)
        {
            DEBUGPRINTF("Graph reference is valid but not a graph\n");
        }
        if (graph->numParams != num)
        {
            DEBUGPRINTF("Error: we were told of %u params but graph has %u\n", num, graph->numParams);
        }
        for (p = 0; p < num && VX_SUCCESS == status; ++p)
        {
            status = vxSetGraphParameterByIndex(graph, p, parameters[p]);
        }
        DEBUGPRINTF("Graph parameters set, status = %d\n", status);
        /* Verify graph (for any user nodes) */
        if (VX_SUCCESS == status)
            status = vxVerifyGraph(graph);
        DEBUGPRINTF("Graph verification complete, status = %d\n", status);
        /* Copy meta-data */
        for (p = 0; p < num && VX_SUCCESS == status; ++p)
        {
            status = vxSetMetaFormatFromReference(metas[p], parameters[p]);
        }
        DEBUGPRINTF("Meta data set, status = %d\n", status);
    }
    node->kernel->attributes.localDataSize = 0;
    return status;
}

static vx_status graph_kernel_init(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;
    (void)parameters;
    (void)num;

    /* Anything to do? */
    DEBUGPRINTF("Graph kernel initialisation\n");
    return VX_SUCCESS;
}

static vx_status graph_kernel_deinit(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;
    (void)parameters;
    (void)num;

    /* Anything to do? */
    DEBUGPRINTF("Graph kernel de-initialisation\n");
    return VX_SUCCESS;
}

static vx_status graph_kernel_function(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    DEBUGPRINTF("Graph kernel function\n");
    /* Set all parameters to the graph, and execute it */
    vx_status status = VX_SUCCESS;
    vx_graph graph = (vx_graph)node->kernel->base.scope;    /* get the graph */
    vx_uint32 p;                                            /* for parameter enumeration */
    /* Set all parameters to graph */
    for (p = 0; p < num && VX_SUCCESS == status; ++p)
    {
        status = vxSetGraphParameterByIndex(graph, p, parameters[p]);
    }
    /* Execute the graph */
    if (VX_SUCCESS == status)
        status = vxProcessGraph(graph);
    return status;
}

static vx_kernel createGraphKernel(vx_context context, vx_graph graph)
{
    /* The graph reference will be stored in the scope of the kernel object.
    This means that, if necessary, the graph can easily be exported and imported
    using existing mechanisms.
    */
    vx_enum kernel_enum;
    vx_uint32 p = 0;
    vx_status status = vxAllocateUserKernelId(context, &kernel_enum);
    vx_kernel kernel = vxAddUserKernel(context, graph->base.name, kernel_enum, (vx_kernel_f)graph_kernel_function, graph->numParams,
                                       (vx_kernel_validate_f)graph_kernel_validate, (vx_kernel_initialize_f)graph_kernel_init, (vx_kernel_deinitialize_f)graph_kernel_deinit);
    DEBUGPRINTF("Graph kernel creation, name = %s, number of parameters = %u\n", graph->base.name, graph->numParams);
    if (VX_SUCCESS == vxGetStatus((vx_reference)kernel))
    {
        kernel->base.scope = (vx_reference)graph;       /* The crucial (and sneaky!) bit. */
        for (p = 0; p < graph->numParams && VX_SUCCESS == status; ++p)
        {
            vx_parameter parameter = vxGetGraphParameterByIndex(graph, p);
            vx_enum direction, type, state;
            vxQueryParameter(parameter, VX_PARAMETER_DIRECTION, &direction, sizeof(direction));
            vxQueryParameter(parameter, VX_PARAMETER_TYPE, &type, sizeof(type));
            vxQueryParameter(parameter, VX_PARAMETER_STATE, &state, sizeof(state));
            status = vxAddParameterToKernel(kernel, p, direction, type, state);
        }
        DEBUGPRINTF("Number of parameters = %u, status = %d\n", graph->numParams, status);
        vxFinalizeKernel(kernel);
    }
    else
    {
        DEBUGPRINTF("? Kernel not a valid reference\n");
    }

    return kernel;
}

#define VX_IX_ID (0x1234C0DE)           /* To identify an export and make sure endian matches etc. */
#define VX_IX_VERSION ((VX_VERSION << 8) + 1)
#define VX_IX_VERSION_1_2 (VX_VERSION_1_2 << 8)

#define IMAGE_SUB_TYPE_NORMAL (0)       /* Code to indicate 'normal' image */
#define IMAGE_SUB_TYPE_UNIFORM (1)      /* Code to indicate uniform image */
#define IMAGE_SUB_TYPE_ROI (2)          /* Code to indocate ROI image */
#define IMAGE_SUB_TYPE_CHANNEL (3)      /* Code to indicate image created from channel */
#define IMAGE_SUB_TYPE_TENSOR (4)       /* Code to indicate an image created from a tensor */
#define IMAGE_SUB_TYPE_HANDLE (5)       /* Code to indicate an image created from handle */

#define VX_IMPORT_TYPE_BINARY (1)       /* Can't be returned to application with IX extension, but if XML
                                           is here as well, may be queried */
struct VXBinHeaderS {                   /* What should be at the start of the binary blob */
    vx_uint32 vx_ix_id;                 /* Unique ID */
    vx_uint32 version;                  /* Version of the export/import */
    vx_uint64 length;                   /* Total length of the blob */
    vx_uint32 numrefs;                  /* Number of references originally supplied */
    vx_uint32 actual_numrefs;           /* Actual number of references exported */
};
typedef struct VXBinHeaderS VXBinHeader;

#define USES_OFFSET (sizeof(VXBinHeader))/* Where the variable part of the blob starts */

struct VXBinObjectHeaderS 
{
    vx_enum type;
    vx_enum use;
    char name[VX_MAX_REFERENCE_NAME];
    vx_uint8 is_virtual;
    vx_uint32 scope;
    const vx_uint8* ptr;
    const vx_uint8* curptr;
};
typedef struct VXBinObjectHeaderS VXBinObjectHeader;

#define getHeader(ptr) ((const VXBinHeader *)ptr)
#define getEnd(ptr) (ptr + getHeader(ptr)->length)
#define getUses(ptr) ((const vx_enum *)(ptr + USES_OFFSET))
#define getOffsets(ptr) ((const vx_uint64 *)(ptr + USES_OFFSET + sizeof(vx_uint32) * getHeader(ptr)->numrefs))
#define checkIndex(ptr, index) (index < getHeader(ptr)->actual_numrefs)
#define checkSize(ptr, curptr, length) (curptr && (curptr + length <= getEnd(ptr)))

static vx_enum getObjectType(const vx_uint8 *ptr, vx_uint32 index)
{
    return checkIndex(ptr, index) ?
        *((const vx_enum *)(((const vx_uint8 *)ptr) + getOffsets(ptr)[index])) :
        VX_TYPE_INVALID;    
}

static const vx_char *getObjectName(const vx_uint8 *ptr, vx_uint32 index)
{
    return checkIndex(ptr, index) ?
        ((const vx_char *)(((const vx_uint8 *)ptr) + getOffsets(ptr)[index] + 2 * sizeof(vx_enum))) :
        "";    
}

static vx_status importObject(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n);

static void importBytes(VXBinObjectHeader *hdr, void * dest, const vx_size length)
{
    if (hdr->curptr)
    {
        if (checkSize(hdr->ptr, hdr->curptr, length))
        {
            memcpy(dest, hdr->curptr, length);
            hdr->curptr += length;
        }
        else
        {
            hdr->curptr = NULL;
        }
    }
}

#define IMPORT_FUNCTION(name, type) \
static VX_INLINE void name(VXBinObjectHeader *hdr, type *data) \
{ \
    importBytes(hdr, data, sizeof(type)); \
}

IMPORT_FUNCTION(importVxUint32, vx_uint32)
IMPORT_FUNCTION(importVxUint16, vx_uint16)
IMPORT_FUNCTION(importVxUint8, vx_uint8)
IMPORT_FUNCTION(importVxInt32, vx_int32)
IMPORT_FUNCTION(importVxInt16, vx_int16)
IMPORT_FUNCTION(importVxFloat32, vx_float32)
IMPORT_FUNCTION(importVxEnum, vx_enum)
IMPORT_FUNCTION(importVxDfImage, vx_df_image)
IMPORT_FUNCTION(importVxBorderT, vx_border_t)
IMPORT_FUNCTION(importVxCoordinates2dT, vx_coordinates2d_t)

static vx_status importObjectHeader(const vx_uint8 *ptr, VXBinObjectHeader *object_header, vx_uint32 index)
{
    /* import the object header for the given index, and return a pointer to the next byte */
    object_header->ptr = ptr;
    object_header->curptr = ptr + getOffsets(ptr)[index];
    importVxEnum(object_header, &object_header->type);
    importVxEnum(object_header, &object_header->use);
    importBytes(object_header, &object_header->name, VX_MAX_REFERENCE_NAME);
    importVxUint8(object_header, &object_header->is_virtual);
    importVxUint32(object_header, &object_header->scope);
    return object_header->curptr ? VX_SUCCESS : VX_FAILURE;
}

static vx_int32 getFirstIndex(const vx_uint8 * ptr, const vx_uint32 index)
{
    /* check to see if there is a duplicate entry in the table at a lower index. Note
    we are guaranteed no duplicates after the first numrefs entries */
    vx_uint32 i;
    vx_uint64 offset = getOffsets(ptr)[index];
    for (i = 0; i < index; ++i)
        if (getOffsets(ptr)[i] == offset)
            break;
    return i;
}

static vx_status importImage(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* Export format:
    vx_uint8 image sub-type
    union {
        struct {    //ROI
            vx_rectangle_t rect
        }
        struct {    //Image from channel
            vx_enum channel
        }
        struct {    //Uniform image
            width, height, format, pixel value
        }
        struct {    //Regular image (but may be part of a pyramid, delay or object array, and it may be virtual)
            width, height, format, data flag
            valid region rectangle and pixel data for each plane (if data flag non-zero)
        }
    }
        (See the corresponding export functions for a full explanation)
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint8 sub_type;
    status = importObjectHeader(ptr, &header, n);
    importVxUint8(&header, &sub_type);
    switch (sub_type)
    {
        case IMAGE_SUB_TYPE_ROI:
            /* First, check that the parent scope is present, and if not, import it */
            if (checkIndex(ptr, header.scope))
            {
                if (NULL == ref_table[header.scope])
                {
                    status = importObject(context, ptr, ref_table, header.scope);
                }
                if (VX_SUCCESS == status)
                {
                    vx_rectangle_t rect;
                    importVxUint32(&header, &rect.start_x);
                    importVxUint32(&header, &rect.start_y);
                    importVxUint32(&header, &rect.end_x);
                    importVxUint32(&header, &rect.end_y);
                    if (NULL == ref_table[n])
                    {
                        ref_table[n] = (vx_reference)vxCreateImageFromROI((vx_image)ref_table[header.scope], &rect);
                        status = vxGetStatus(ref_table[n]);
                    }
                    else /* check meta-data */
                    {
                        vx_image image = (vx_image)ref_table[n];
                        if (image->width != rect.end_x - rect.start_x ||
                            image->height != rect.end_y - rect.start_y ||
                            image->format != ((vx_image)ref_table[header.scope])->format)
                        {
                            status = VX_ERROR_INVALID_PARAMETERS;
                        }
                    }
                }
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;   /* index out of bounds */
            }

            break;
        case IMAGE_SUB_TYPE_CHANNEL:
            /* First, check that the parent scope is present, and if not, import it */
            if (checkIndex(ptr, header.scope))
            {
                if (NULL == ref_table[header.scope])
                {
                    status = importObject(context, ptr, ref_table, header.scope);
                }
                if (VX_SUCCESS == status)
                {
                    vx_enum channel;
                    importVxEnum(&header, &channel);
                    if (NULL == ref_table[n])
                    {
                        ref_table[n] = (vx_reference)vxCreateImageFromChannel((vx_image)ref_table[header.scope], channel);
                        status = vxGetStatus(ref_table[n]);
                    }
                    else /* check meta-data */
                    {
                        vx_image image = (vx_image)ref_table[n];
                        if (image->width != ((vx_image)ref_table[header.scope])->width ||
                            image->height != ((vx_image)ref_table[header.scope])->height ||
                            image->format != VX_DF_IMAGE_U8)
                        {
                            status = VX_ERROR_INVALID_PARAMETERS;
                        }
                    }
                }
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;   /* index out of bounds */
            }
            break;
        case IMAGE_SUB_TYPE_UNIFORM:
            {
                vx_uint32 width, height;
                vx_df_image format;
                importVxUint32(&header, &width);
                importVxUint32(&header, &height);
                importVxDfImage(&header, &format);
                if (NULL == ref_table[n])
                {
                    ref_table[n] = (vx_reference)vxCreateUniformImage(context, width, height, format,
                                                                    (const vx_pixel_value_t *)header.curptr);
                    status = vxGetStatus(ref_table[n]);
                }
                else    /* check the meta-data */
                {
                    vx_image image = (vx_image)ref_table[n];
                    if (image->width != width ||
                        image->height != height ||
                        image->format != format)
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                    }
                }
            }
            break;
        case IMAGE_SUB_TYPE_HANDLE:     /* bugzilla 16312 */
            {
                vx_uint32 width, height, planes;
                vx_imagepatch_addressing_t addrs[VX_PLANE_MAX];
                void *ptrs[VX_PLANE_MAX];
                vx_df_image format;
                vx_enum memory_type;
                importVxUint32(&header, &width);
                importVxUint32(&header, &height);
                importVxDfImage(&header, &format);
                importVxUint32(&header, &planes);
                importVxEnum(&header, &memory_type);
                if (NULL == ref_table[n])
                {
                    vx_uint32 p;
                    for (p = 0; p < planes; ++p)
                    {
                        vx_int32 stride_x, stride_y;
                        importVxInt32(&header, &stride_x);
                        importVxInt32(&header, &stride_y);
                        addrs[p].stride_x = stride_x;
                        addrs[p].stride_y = stride_y;
                        addrs[p].dim_x = width;
                        addrs[p].dim_y = height;
                        ptrs[p] = NULL;
                    }
                    ref_table[n] = (vx_reference)vxCreateImageFromHandle(context, format, addrs, ptrs, memory_type);
                    status = vxGetStatus(ref_table[n]);
                }
            }
            break;
        case IMAGE_SUB_TYPE_NORMAL:
            {
                DEBUGPRINTF("Importing image index %u\n", n);
                vx_uint32 width, height, data = 0;
                vx_df_image format;
                importVxUint32(&header, &width);
                importVxUint32(&header, &height);
                importVxDfImage(&header, &format);
                importVxUint32(&header, &data);
                if (NULL == ref_table[n])
                {
                    if (header.is_virtual)
                    {
                        if (checkIndex(ptr, header.scope))
                        {
                            ref_table[n] = (vx_reference)vxCreateVirtualImage((vx_graph)ref_table[header.scope],
                                            width, height, format);
                        }
                    }
                    else
                    {
                        ref_table[n] = (vx_reference)vxCreateImage(context, width, height, format);
                    }
                    status = vxGetStatus(ref_table[n]);
                }
                else    /* check the meta-data */
                {
                    vx_image image = (vx_image)ref_table[n];
                    if (image->width != width ||
                        image->height != height ||
                        image->format != format)
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                    }
                }
                /* now we have to import data, if there is any */
                if (data && VX_SUCCESS == status && !header.is_virtual)
                {
                    vx_image image = (vx_image)ref_table[n];
                    vx_uint32 p;
                    /* Get the valid rectangle */
                    vx_rectangle_t rect;
                    importVxUint32(&header, &rect.start_x);
                    importVxUint32(&header, &rect.start_y);
                    importVxUint32(&header, &rect.end_x);
                    importVxUint32(&header, &rect.end_y);
                    status = vxSetImageValidRectangle(image, &rect);
                    DEBUGPRINTF("Importing image data for rect (start_x=%u, end_x=%u, start_y=%u, end_y=%u)\n", 
                                rect.start_x, rect.end_x, rect.start_y, rect.end_y);
                    for (p = 0U; p < image->planes && VX_SUCCESS == status; ++p)
                    {
                        void *base = NULL;
                        vx_uint32 x, y;
                        vx_imagepatch_addressing_t addr;
                        vx_map_id id;
                        status = vxMapImagePatch(image, &rect, p, &id, &addr, &base,
                                                    VX_WRITE_ONLY, image->memory_type, 0);
                        for (y = 0U; y < addr.dim_y; y += addr.step_y)
                        {
                            for( x = 0U; x < addr.dim_x; x += addr.step_x)
                            {
                                /* import a single pixel */
                                vx_pixel_value_t *pix = (vx_pixel_value_t *)vxFormatImagePatchAddress2d(base, x, y, &addr);
                                switch (format)
                                {
                                    case VX_DF_IMAGE_U8:    /* single-plane 1 byte */
                                        importVxUint8(&header, &pix->U8);
                                        break;
                                    case VX_DF_IMAGE_YUV4:  /* 3-plane 1 byte */
                                    case VX_DF_IMAGE_IYUV:
                                        importVxUint8(&header, &pix->YUV[p]);
                                        break;
                                    case VX_DF_IMAGE_U16:   /* single-plane U16 */
                                        importVxUint16(&header, &pix->U16);
                                        break;
                                    case VX_DF_IMAGE_S16:   /* single-plane S16 */
                                        importVxInt16(&header, &pix->S16);
                                        break;
                                    case VX_DF_IMAGE_U32:   /* single-plane U32 */
                                        importVxUint32(&header, &pix->U32);
                                        break;
                                    case VX_DF_IMAGE_S32:   /* single-plane S32 */
                                        importVxInt32(&header, &pix->S32);
                                        break;
                                    case VX_DF_IMAGE_RGB:   /* single-plane 3 bytes */
                                        importBytes(&header, pix->RGB, 3);
                                        break;
                                    case VX_DF_IMAGE_RGBX:  /* single-plane 4 bytes */
                                        importBytes(&header, pix->RGBX, 4);
                                        break;
                                    case VX_DF_IMAGE_UYVY:  /* single plane, import 2 bytes for UY or VY */
                                    case VX_DF_IMAGE_YUYV:  /* single plane, import 2 bytes for YU or YV */
                                        importBytes(&header, pix->YUV, 2);
                                        break;
                                    case VX_DF_IMAGE_NV12:  /* import 1 byte from 1st plane and 2 from second */
                                    case VX_DF_IMAGE_NV21:  /* second plane order is different for the two formats */
                                        if (0 == p)
                                        {
                                            importVxUint8(&header, &pix->YUV[0]);
                                        }
                                        else
                                        {
                                            importVxUint8(&header, &pix->YUV[0]);
                                            importVxUint8(&header, &pix->YUV[1]);
                                        }
                                        break;
                                    default:
                                        status = VX_ERROR_NOT_SUPPORTED;
                                }
                            }
                        }
                        status |= vxUnmapImagePatch(image, id);
                    }
                }
            }
            break;
        case IMAGE_SUB_TYPE_TENSOR:
            /* Here for an image that is derived from a tensor.
                These have already been created by the parent object array,
                so nothing to do.
                */
            break;
        default:
            status = VX_ERROR_NOT_SUPPORTED;
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importLUT(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* 
    LUT format:
    vx_uint32 num_items
    vx_enum item_type
    vx_uint32 offset
    vx_uint32 size
    vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 num_items, offset, size;
    vx_enum item_type;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &num_items);
    importVxEnum(&header, &item_type);
    importVxUint32(&header, &offset);
    importVxUint32(&header, &size);
    if (NULL == ref_table[n])
    {   /* create object */
        ref_table[n] = (vx_reference)vxCreateLUT(context, item_type, num_items);
        if (header.is_virtual)
        {
            if (checkIndex(ptr, header.scope))
            {
                ref_table[n]->scope = (vx_reference)ref_table[header.scope];
                ref_table[n]->is_virtual = vx_true_e;
            }
            else
                status = VX_ERROR_INVALID_SCOPE;
        }
        status = status || vxGetStatus(ref_table[n]);
    }
    else /* Just check meta-data */
    {
        vx_lut_t *lut = (vx_lut_t *)ref_table[n];
        if (VX_TYPE_LUT != ref_table[n]->type ||
            lut->num_items != num_items ||
            lut->offset != offset ||
            lut->item_type != item_type)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    if (VX_SUCCESS == status && size)
    {   /* read data */
        vx_lut_t *lut = (vx_lut_t *)ref_table[n];
        importBytes(&header, lut->memory.ptrs[0], size);
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importDistribution(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /*
    Distribution format:
    vx_uint32 bins
    vx_int32 offset_x
    vx_uint32 range_x
    vx_uint32 size
    vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 bins, range_x, size;
    vx_int32 offset_x;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &bins);
    importVxInt32(&header, &offset_x);
    importVxUint32(&header, &range_x);
    importVxUint32(&header, &size);
    if (NULL == ref_table[n])
    {   /* create object */
        ref_table[n] = (vx_reference)vxCreateDistribution(context, bins, offset_x, range_x);
        if (header.is_virtual)
        {
            if (checkIndex(ptr, header.scope))
            {
                ref_table[n]->scope = (vx_reference)ref_table[header.scope];
                ref_table[n]->is_virtual = vx_true_e;
            }
            else
                status = VX_ERROR_INVALID_SCOPE;
        }
        status = status || vxGetStatus(ref_table[n]);
    }
    else /* Just check meta-data */
    {
        vx_distribution dist = (vx_distribution)ref_table[n];
        if (VX_TYPE_DISTRIBUTION != ref_table[n]->type ||
            dist->memory.dims[0][VX_DIM_X] != bins ||
            dist->offset_x != offset_x ||
            dist->range_x != range_x)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    if (VX_SUCCESS == status && size)
    {   /* read data */
        vx_distribution dist = (vx_distribution)ref_table[n];
        importBytes(&header, dist->memory.ptrs[0], size);
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importThreshold(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /*
    Threshold format:
    vx_enum thresh_type
    vx_enum data_type
    vx_int32 value
    vx_int32 lower
    vx_int32 upper
    vx_int32 true_value
    vx_int32 false_value
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    status = importObjectHeader(ptr, &header, n);
    vx_enum thresh_type;
    vx_enum data_type;
    vx_int32 value;
    vx_int32 lower;
    vx_int32 upper;
    vx_int32 true_value;
    vx_int32 false_value;
    importVxEnum(&header, &thresh_type);
    importVxEnum(&header, &data_type);
    importVxInt32(&header, &value);
    importVxInt32(&header, &lower);
    importVxInt32(&header, &upper);
    importVxInt32(&header, &true_value);
    importVxInt32(&header, &false_value);
    status = VX_ERROR_NOT_SUPPORTED;
    if (NULL == ref_table[n])
    {
        vx_threshold threshold = (vx_threshold)ownCreateReference(context, VX_TYPE_THRESHOLD, VX_EXTERNAL, &context->base);
        status = vxGetStatus((vx_reference)threshold);
        if (VX_SUCCESS == status && threshold->base.type == VX_TYPE_THRESHOLD)
        {
            threshold->thresh_type = thresh_type;
            threshold->data_type = data_type;
            threshold->value.S32 = value;
            threshold->lower.S32 = lower;
            threshold->upper.S32 = upper;
            threshold->true_value.S32 = true_value;
            threshold->false_value.S32 = false_value;
            threshold->input_format = VX_DF_IMAGE_U8;
            threshold->output_format = VX_DF_IMAGE_U8;
            if (header.is_virtual)
            {
                if (checkIndex(ptr, header.scope))
                {
                    threshold->base.scope = ref_table[header.scope];
                    threshold->base.is_virtual = vx_true_e;
                }
                else
                    status = VX_ERROR_INVALID_SCOPE;
            }
            ref_table[n] = (vx_reference)threshold;
        }
    }
    else
    {
        vx_threshold threshold = (vx_threshold)ref_table[n];
        if (VX_TYPE_THRESHOLD != ref_table[n]->type ||
            threshold->thresh_type != thresh_type ||
            threshold->data_type != data_type ||
            (VX_THRESHOLD_TYPE_BINARY == thresh_type &&
                threshold->value.S32 != value) ||
            (VX_THRESHOLD_TYPE_RANGE == thresh_type &&
                (threshold->lower.S32 != lower ||
                threshold->upper.S32 != upper)) ||
            threshold->true_value.S32 != true_value ||
            threshold->false_value.S32 != false_value)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importMatrix(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /*
    Matrix format:
    vx_uint32 columns
    vx_uint32 rows
    vx_enum data_type
    vx_coordinates2d_t origin
    vx_enum pattern
    vx_uint32 size
    vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 columns, rows, size;
    vx_enum data_type, pattern;
    vx_coordinates2d_t origin;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &columns);
    importVxUint32(&header, &rows);
    importVxEnum(&header, &data_type);
    importVxCoordinates2dT(&header, &origin);
    importVxEnum(&header, &pattern);
    importVxUint32(&header, &size);
    if (NULL == ref_table[n])
    {   /* create object */
        ref_table[n] = (vx_reference)vxCreateMatrix(context, data_type, columns, rows);
        if (header.is_virtual)
        {
            if (checkIndex(ptr, header.scope))
            {
                ref_table[n]->scope = (vx_reference)ref_table[header.scope];
                ref_table[n]->is_virtual = vx_true_e;
            }
            else
                status = VX_ERROR_INVALID_SCOPE;
        }
        status = status || vxGetStatus(ref_table[n]);
    }
    else /* Just check meta-data */
    {
        vx_matrix matrix = (vx_matrix)ref_table[n];
        if (VX_TYPE_MATRIX != ref_table[n]->type ||
            matrix->columns != columns ||
            matrix->rows != rows ||
            matrix->data_type != data_type ||
            matrix->pattern != pattern ||
            matrix->origin.x != origin.x ||
            matrix->origin.y != origin.y )
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    if (VX_SUCCESS == status && size)
    {   /* read data */
        vx_matrix matrix = (vx_matrix)ref_table[n];
        importBytes(&header, matrix->memory.ptrs[0], size);
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importConvolution(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /*
    Convolution format:
    vx_uint32 columns
    vx_uint32 rows
    vx_uint32 scale
    vx_uint32 size
    vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 columns, rows, scale, size;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &columns);
    importVxUint32(&header, &rows);
    importVxUint32(&header, &scale);
    importVxUint32(&header, &size);
    if (NULL == ref_table[n])
    {   /* create object */
        ref_table[n] = (vx_reference)vxCreateConvolution(context, columns, rows);
        if (header.is_virtual)
        {
            if (checkIndex(ptr, header.scope))
            {
                ref_table[n]->scope = (vx_reference)ref_table[header.scope];
                ref_table[n]->is_virtual = vx_true_e;
            }
            else
                status = VX_ERROR_INVALID_SCOPE;
        }
        status = status || vxGetStatus(ref_table[n]);
        status = status || vxSetConvolutionAttribute((vx_convolution)ref_table[n],
                                                    VX_CONVOLUTION_SCALE, &scale, sizeof(scale));
    }
    else /* Just check meta-data */
    {
        vx_convolution convolution = (vx_convolution)ref_table[n];
        if (VX_TYPE_CONVOLUTION != ref_table[n]->type ||
            convolution->base.columns != columns ||
            convolution->base.rows != rows ||
            convolution->scale != scale)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    if (VX_SUCCESS == status && size)
    {   /* read data */
        vx_convolution convolution = (vx_convolution)ref_table[n];
        importBytes(&header, convolution->base.memory.ptrs[0], size);
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importScalar(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* 
    Format:
        vx_enum data_type
        vx_uint32 item_size
        if (VX_USER_STRUCT_START <= data_type <= VX_USER_STRUCT_END)
            vx_char struct_name[VX_MAX_STRUCT_NAME]
        vx_uint32 size              # Size of data exported
        vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 size, item_size;
    vx_enum data_type;
    status = importObjectHeader(ptr, &header, n);
    importVxEnum(&header, &data_type);
    importVxUint32(&header, &item_size);
    if (VX_TYPE_USER_STRUCT_START <= data_type &&    /* Now import the name if it's a user struct (Bugzilla 16338)*/
        VX_TYPE_USER_STRUCT_END >= data_type)
    {
        data_type = vxGetUserStructByName(context, (const vx_char *)header.curptr);
        header.curptr += VX_MAX_STRUCT_NAME;
        if (VX_TYPE_INVALID == data_type ||
            item_size != context->user_structs[data_type - VX_TYPE_USER_STRUCT_START].size)
            status = VX_ERROR_INVALID_TYPE;
    }
    if (VX_SUCCESS == status)
    {
        importVxUint32(&header, &size);
        if (NULL == ref_table[n])
        {   /* create object */
            ref_table[n] = (vx_reference)vxCreateScalar(context, data_type, header.curptr);
            if (header.is_virtual)
            {
                if (checkIndex(ptr, header.scope))
                {
                    ref_table[n]->scope = (vx_reference)ref_table[header.scope];
                    ref_table[n]->is_virtual = vx_true_e;
                }
                else
                    status = VX_ERROR_INVALID_SCOPE;
            }
            status = status || vxGetStatus(ref_table[n]);
        }
        else /* Just check meta-data */
        {
            vx_scalar scalar = (vx_scalar)ref_table[n];
            if (VX_TYPE_SCALAR != ref_table[n]->type ||
                scalar->data_type != data_type)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
        }
    }
    if (VX_SUCCESS == status && size)
    {
        vx_uint8 *data = malloc(size);
        if (data)
        {
            importBytes(&header, data, size);
            status = vxCopyScalar((vx_scalar)ref_table[n], data, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            free(data);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importArray(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* 
    Array format:
        vx_uint32 num_items
        vx_uint32 capacity
        vx_enum item_type
        vx_uint32 item_size
        if (VX_USER_STRUCT_START <= item_type <= VX_USER_STRUCT_END)    # Not for version < 1.2!
            vx_char struct_name[VX_MAX_STRUCT_NAME]
        vx_uint32 size              # Size of data exported
        vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 num_items, capacity, item_size, size;
    vx_enum item_type;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &num_items);
    importVxUint32(&header, &capacity);
    importVxEnum(&header, &item_type);
    importVxUint32(&header, &item_size);
    if (VX_TYPE_USER_STRUCT_START <= item_type &&    /* Now import the name if it's a user struct (Bugzilla 16338)*/
        VX_TYPE_USER_STRUCT_END >= item_type)
    {
        item_type = vxGetUserStructByName(context, (const vx_char *)header.curptr);
        header.curptr += VX_MAX_STRUCT_NAME;
        if (VX_TYPE_INVALID == item_type ||
            item_size != context->user_structs[item_type - VX_TYPE_USER_STRUCT_START].size)
            status = VX_ERROR_INVALID_TYPE;
    }
    if (VX_SUCCESS == status)
    {
        importVxUint32(&header, &size);
        if (NULL == ref_table[n])
        {   /* create object */
            if (header.is_virtual)
            {
                if (checkIndex(ptr, header.scope))
                {
                    ref_table[n] = (vx_reference)vxCreateVirtualArray((vx_graph)ref_table[header.scope], item_type, capacity);
                }
            }
            else
                ref_table[n] = (vx_reference)vxCreateArray(context, item_type, capacity);
            status = vxGetStatus(ref_table[n]);
        }
        else /* Just check meta-data */
        {
            vx_array array = (vx_array)ref_table[n];
            if (VX_TYPE_ARRAY != ref_table[n]->type ||
                array->capacity != capacity ||
                array->item_size != item_size ||
                array->item_type != item_type)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
        }
    }
    if (VX_SUCCESS == status && size)
    {   /* read data */
        vx_array array = (vx_array)ref_table[n];
        importBytes(&header, array->memory.ptrs[0], size);
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importRemap(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* 
    Remap format:
    vx_uint32 src_width
    vx_uint32 src_height
    vx_uint32 dst_width
    vx_uint32 dst_height
    vx_uint32 size
    vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 src_width, src_height, dst_width, dst_height, size;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &src_width);
    importVxUint32(&header, &src_height);
    importVxUint32(&header, &dst_width);
    importVxUint32(&header, &dst_height);
    importVxUint32(&header, &size);
    if (NULL == ref_table[n])
    {   /* create object */
        ref_table[n] = (vx_reference)vxCreateRemap(context, src_width, src_height, dst_width, dst_height);
        if (header.is_virtual)
        {
            if (checkIndex(ptr, header.scope))
            {
                ref_table[n]->scope = (vx_reference)ref_table[header.scope];
                ref_table[n]->is_virtual = vx_true_e;
            }
            else
                status = VX_ERROR_INVALID_SCOPE;
        }
        status = status || vxGetStatus(ref_table[n]);
    }
    else /* Just check meta-data */
    {
        vx_remap remap = (vx_remap)ref_table[n];
        if (VX_TYPE_REMAP != ref_table[n]->type ||
            remap->src_width != src_width ||
            remap->src_height != src_height ||
            remap->dst_width != dst_width ||
            remap->dst_height != dst_height)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    if (VX_SUCCESS == status && size)
    {   /* read data */
        vx_remap remap = (vx_remap)ref_table[n];
        importBytes(&header, remap->memory.ptrs[0], size);
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importObjectArray(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n, vx_enum required_type)
{
    /* Object array format:
    vx_uint32 num_items
    vx_enum parent_type
    if VX_TYPE_TENSOR == parent_type
        vx_uint32 tensor reference, stride, rect.start_x, rect.start_y, rect.end_x. rect.end_y
        vx_df_image format
    vx_uint32 indices[num_items]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 num_items;
    vx_uint32 c;
    vx_enum parent_type;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &num_items);
    importVxEnum(&header, &parent_type);
    
    if (required_type != parent_type)
    {
        return status;      /* quick exit if we don't want to import this at present */
    }
    
    if (NULL == ref_table[n])
    {
        if (VX_TYPE_TENSOR == parent_type)
        {
            vx_rectangle_t rect;
            vx_uint32 tensor_ix;
            vx_uint32 stride;
            vx_df_image format;
            importVxUint32(&header, &tensor_ix);
            importVxUint32(&header, &stride);
            importVxUint32(&header, &rect.start_x);
            importVxUint32(&header, &rect.start_y);
            importVxUint32(&header, &rect.end_x);
            importVxUint32(&header, &rect.end_y);
            importVxDfImage(&header, &format);
            if (checkIndex(ptr, tensor_ix))
            {
                if (NULL == ref_table[tensor_ix])
                {
                    status = importObject(context, ptr, ref_table, tensor_ix);
                }
            }
            else
            {
                status = VX_FAILURE;
                DEBUGPRINTF("tensor index check failed\n");
            }
            if (VX_SUCCESS == status)
            {
                ref_table[n] = (vx_reference)vxCreateImageObjectArrayFromTensor((vx_tensor)ref_table[tensor_ix], &rect, num_items, stride, format);
                status = vxGetStatus(ref_table[n]);
            }
        }
        else
        if (VX_TYPE_INVALID == parent_type)
        {
            vx_uint32 exemplar_ix;
            const vx_uint8 *ixcurptr = header.curptr;
            importVxUint32(&header, &exemplar_ix);        /* First item in array as exemplar, don't increment curptr */
            header.curptr = ixcurptr;
            if (checkIndex(ptr, exemplar_ix) && NULL == ref_table[exemplar_ix])
            {
                status = importObject(context, ptr, ref_table, exemplar_ix);    /* import exemplar */
                if (VX_SUCCESS == status)
                {
                    if (header.is_virtual)
                    {
                        if (checkIndex(ptr, header.scope))
                        {
                            ref_table[n] = (vx_reference)vxCreateVirtualObjectArray((vx_graph)ref_table[header.scope],
                                                        ref_table[exemplar_ix], num_items);
                        }
                    }
                    else
                        ref_table[n] = (vx_reference)vxCreateObjectArray(context, ref_table[exemplar_ix], num_items);
                    status = vxGetStatus(ref_table[n]);
                    vxReleaseReference(&ref_table[exemplar_ix]);/* Release the exemplar */
                }
                ref_table[exemplar_ix] = NULL;              /* make entry zero as we will set it to new value */
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;       /* References in an object array can't be supplied by user */
            }
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;       /* parent type must be INVALID or TENSOR */
            DEBUGPRINTF("parent was neither INVALID nor TENSOR\n");
        }
    }
    else if (VX_TYPE_OBJECT_ARRAY != ref_table[n]->type ||
            num_items != ((vx_object_array)ref_table[n])->num_items)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    for (c = 0; c < num_items && VX_SUCCESS == status; ++c)
    {
        /* import each object - note that as they have already been created, only a meta-data check and
        data import will be done. We do not increment the reference for the objects in the delay - this
        will be done at the end of the main function if the objects are on the import list */
        vx_uint32 object_ix;
        importVxUint32(&header, &object_ix);
        if (checkIndex(ptr, object_ix))
        {
            ref_table[object_ix] = ((vx_object_array)ref_table[n])->items[c];
            if (object_ix > getHeader(ptr)->numrefs)
                vxRetainReference(ref_table[object_ix]); /* need to fix this up becuase of a release later */
            status = importObject(context, ptr, ref_table, object_ix);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importPyramid(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* Pyramid format:
    vx_uint32 width
    vx_uint32 height
    vx_df_image format
    vx_float32 scale
    vx_uint32 #levels
    vx_uint32 levels[#levels]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 width, height;
    vx_df_image format;
    vx_float32 scale;
    vx_uint32 num_levels;
    status = importObjectHeader(ptr, &header, n);
    vx_uint32 c;
    importVxUint32(&header, &width);
    importVxUint32(&header, &height);
    importVxDfImage(&header, &format);
    importVxFloat32(&header, &scale);
    importVxUint32(&header, &num_levels);
    if (NULL == ref_table[n])
    {
        if (header.is_virtual)
        {
            if (checkIndex(ptr, header.scope))
            {
                ref_table[n] = (vx_reference)vxCreateVirtualPyramid((vx_graph)ref_table[header.scope], num_levels, 
                                                                scale, width, height, format);
            }
        }
        else
            ref_table[n] = (vx_reference)vxCreatePyramid(context, num_levels, scale, width, height, format);
        status = vxGetStatus(ref_table[n]);
    }
    else    /* Check meta-data */
    {
        vx_pyramid pyramid = (vx_pyramid)ref_table[n];
        if (VX_TYPE_PYRAMID != ref_table[n]->type ||
            pyramid->width != width ||
            pyramid->height != height ||
            pyramid->format != format ||
            pyramid->numLevels != num_levels ||
            pyramid->scale != scale)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    for (c = 0; c < num_levels && VX_SUCCESS == status; ++c)
    {
        /* Import each level - notice that importImage will import only the data since the image has been created.
            We do not increment the reference count for the images; they will be incremented if and only if they
            are on the export list, at the end of the main function. */
        vx_uint32 level_ix;
        importVxUint32(&header, &level_ix);
        if (checkIndex(ptr, level_ix))
        {
            ref_table[level_ix] = (vx_reference)(((vx_pyramid)ref_table[n])->levels[c]);
            status = importImage(context, ptr, ref_table, level_ix);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importDelay(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* Delay format:
    vx_uint32 num_slots
    vx_uint32 indices[num_slots]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 num_slots;
    vx_uint32 c;
    vx_uint32 exemplar_ix;
    const vx_uint8 *ixcurptr;
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &num_slots);
    ixcurptr = header.curptr;
    importVxUint32(&header, &exemplar_ix);            /* First item in array as examplar, don't increment curptr */
    header.curptr = ixcurptr;
    if (NULL == ref_table[n])
    {
        if (checkIndex(ptr, exemplar_ix) && NULL == ref_table[exemplar_ix])
        {
            status = importObject(context, ptr, ref_table, exemplar_ix);    /* import exemplar */
            if (VX_SUCCESS == status)
            {
                if (header.is_virtual)
                    status = VX_ERROR_INVALID_PARAMETERS;   /* Virtual delays not allowed */
                else
                {
                    ref_table[n] = (vx_reference)vxCreateDelay(context, ref_table[exemplar_ix], num_slots);
                    status = vxGetStatus(ref_table[n]);
                }
                vxReleaseReference(&ref_table[exemplar_ix]);/* Release the exemplar */
            }
            ref_table[exemplar_ix] = NULL;              /* make entry zero as we will set it to new value */
        }
        else
            status = VX_ERROR_INVALID_PARAMETERS;       /* References in an object array can't be supplied by user */
    }
    else if (VX_TYPE_DELAY != ref_table[n]->type ||
            num_slots != ((vx_delay)ref_table[n])->count)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    for (c = 0; c < num_slots && VX_SUCCESS == status; ++c)
    {
        /* import each object - note that as they have already been created, only a meta-data check and
        data import will be done.  We do not increment the reference count for the objects in the slots;
        they will be incremented if and only if they are on the export list, at the end of the main function. 
        */
        vx_uint32 object_ix;
        importVxUint32(&header, &object_ix);
        if (checkIndex(ptr, object_ix))
        {
            ref_table[object_ix] = ((vx_delay)ref_table[n])->refs[c];
            status = importObject(context, ptr, ref_table, object_ix);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importTensor(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /*
    Format of tensor export:
    vx_uint32 number_of_dimensions
    vx_enum data_type
    vx_uint8 fixed_point_position
    vx_uint32 memory_offset
    vx_uint32 dimensions[number_of_dimensions]
    vx_uint32 stride[number_of_dimensions]
    vx_uint32 size
    vx_uint8 data[size]
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_uint32 number_of_dimensions, memory_offset, size, dim;
    vx_enum data_type;
    vx_uint8 fixed_point_position;
    vx_size dimensions[VX_MAX_TENSOR_DIMENSIONS], stride[VX_MAX_TENSOR_DIMENSIONS];
    status = importObjectHeader(ptr, &header, n);
    importVxUint32(&header, &number_of_dimensions);
    importVxEnum(&header, &data_type);
    importVxUint8(&header, &fixed_point_position);
    importVxUint32(&header, &memory_offset);
    if (0U == number_of_dimensions || VX_MAX_TENSOR_DIMENSIONS < number_of_dimensions)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    else
    {
        for (dim = 0; dim < number_of_dimensions; ++dim)
        {
            vx_uint32 dimension;
            importVxUint32(&header, &dimension);
            dimensions[dim] = dimension;
        }
        for (dim = 0; dim < number_of_dimensions; ++dim)
        {
            vx_uint32 dim_stride;
            importVxUint32(&header, &dim_stride);
            stride[dim] = dim_stride;
        }
        importVxUint32(&header, &size);
        if (NULL == ref_table[n])
        {
            /* Now check to see if this is a tensor from view */
            if (VX_TYPE_TENSOR == getObjectType(ptr, header.scope))
            {
                /* First, make sure parent is imported */
                if (NULL == ref_table[header.scope])
                    status = importObject(context, ptr, ref_table, header.scope);
                if (VX_SUCCESS == status)
                {
                    vx_size starts[VX_MAX_TENSOR_DIMENSIONS], ends[VX_MAX_TENSOR_DIMENSIONS];
                    /* Calculate starts and ends using the offset, strides, and dimensions */
                    dim = number_of_dimensions - 1;
                    do {
                        starts[dim] = memory_offset / stride[dim];
                        memory_offset -= starts[dim] * stride[dim];
                        ends[dim] = starts[dim] + dimensions[dim];
                    } while (dim--);
                    ref_table[n] = (vx_reference)vxCreateTensorFromView((vx_tensor)ref_table[header.scope],
                                                                    number_of_dimensions, starts, ends);
                    status = vxGetStatus(ref_table[n]);
                }
            }
            else
            {
                if (header.is_virtual)
                {
                    if (checkIndex(ptr, header.scope))
                    {
                            ref_table[n] = (vx_reference)vxCreateVirtualTensor((vx_graph)ref_table[header.scope],
                                                                    number_of_dimensions, dimensions, data_type,
                                                                    fixed_point_position);
                    }
                }
                else
                {
                    ref_table[n] = (vx_reference)vxCreateTensor(context, number_of_dimensions, dimensions, data_type,
                                                                fixed_point_position);
                }
                status = vxGetStatus(ref_table[n]);
            }
        }
        else if (VX_TYPE_TENSOR != ref_table[n]->type ||    /* Check meta-data */
                number_of_dimensions != ((vx_tensor)ref_table[n])->number_of_dimensions ||
                data_type != ((vx_tensor)ref_table[n])->data_type ||
                fixed_point_position != ((vx_tensor)ref_table[n])->fixed_point_position)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        else
        {       /* Most meta-data correct, check each dimension */
            for (dim = 0; dim < number_of_dimensions && VX_SUCCESS == status; ++dim)
            {
                if (((vx_tensor)ref_table[n])->dimensions[dim] != dimensions[dim] ||
                    ((vx_tensor)ref_table[n])->stride[dim] != stride[dim])
                    status = VX_ERROR_INVALID_PARAMETERS;
            }
        }
        if (VX_SUCCESS == status && size && !header.is_virtual)
        {   /* read data */
            vx_size starts[VX_MAX_TENSOR_DIMENSIONS] = {0};
            status = vxCopyTensorPatch((vx_tensor)ref_table[n], number_of_dimensions, starts, dimensions,
                                        stride, (void *)header.curptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
        }
    }
    return header.curptr ? status : VX_FAILURE;
}

static vx_status importKernel(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    /* See bugzilla 16337 */
    /* Format of data:
            vx_enum enumeration                 # kernel enumeration value
            vx_char[VX_MAX_KERNEL_NAME] name    # kernel name
            vx_uint32 num_kernel_params         # number of parameters
            struct
                vx_enum direction
                vx_enum type
                vx_enum state
            end_struct params[num_kernel_params]
            vx_uint32 graph_index               # Only for 'graph kernels' with VX_IX_USE_IMPORT_AS_KERNEL (Bugzilla 16207)
            
    This function is called twice for each kernel, once with required_use == VX_IX_USE_APPLICATION_CREATE and once with VX_IX_USE_IMPORT_AS_KERNEL.
    This is because in the first case the kernels are required before graphs are built, and in the second graphs must be built before the
    kernel can be created.
    */
    vx_status status = VX_SUCCESS;
    VXBinObjectHeader header;
    vx_enum enumeration;
    vx_char name[VX_MAX_KERNEL_NAME];
    vx_uint32 num_kernel_params, p;
    vx_enum directions[VX_INT_MAX_PARAMS];
    vx_enum types[VX_INT_MAX_PARAMS];
    vx_enum states[VX_INT_MAX_PARAMS];
    status = importObjectHeader(ptr, &header, n);
    if (VX_SUCCESS == status)
    {
        importVxEnum(&header, &enumeration);
        importBytes(&header, &name, VX_MAX_KERNEL_NAME);
        importVxUint32(&header, &num_kernel_params);
        DEBUGPRINTF("Importing Kernel at index %u (%s, %u)\n", n, name, enumeration);
        for (p = 0; p < num_kernel_params; ++p)
        {
            importVxEnum(&header, &directions[p]);
            importVxEnum(&header, &types[p]);
            importVxEnum(&header, &states[p]);
        }
        if (NULL == ref_table[n])
        {
            /* Not already in place, we must find the kernel */
            if (VX_KERNEL_BASE(VX_ID_USER,0) == (VX_KERNEL_BASE(VX_ID_USER,0) & enumeration))
            {
		        /* Must be a user kernel */
		        DEBUGPRINTF("Looking for kernel by name\n");
		        ref_table[n] = (vx_reference)vxGetKernelByName(context, name);
            }
            else
            {
                /* Not a user kernel; find by enum */
                DEBUGPRINTF("Looking for kernel by enumeration\n");
                ref_table[n] = (vx_reference)vxGetKernelByEnum(context, enumeration);
            }
            status = status || vxGetStatus(ref_table[n]);
        }
        if (VX_SUCCESS == status)
        {
            /* We check the data. Either name matches because we have found the
            kernel by name, or the kernel is not a user kernel or has been supplied
            and the name may not match.
            Currently the name is not checked here, but parameters must match in
            number, type, direction and state.
            */
            vx_kernel kernel = (vx_kernel)ref_table[n];
            if (num_kernel_params == kernel->signature.num_parameters)
            {
                for (p = 0; p < num_kernel_params; ++p)
                {
                    if (directions[p] != kernel->signature.directions[p] ||
                        types[p] != kernel->signature.types[p] ||
                        states[p] != kernel->signature.states[p])
                    {
                        status = VX_ERROR_NOT_COMPATIBLE;
                        break;
                    }
                }
            }
            else
            {
                DEBUGPRINTF("Kernel not compatible\n");
                status = VX_ERROR_NOT_COMPATIBLE;
            }
        }
        else
        {
            DEBUGPRINTF("Failed to create a kernel\n");
        }
    }
    return header.curptr ? status : VX_FAILURE;;
}

static vx_status importObject(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_uint32 n)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 first = getFirstIndex(ptr, n);
    if (first < n)
    {
        ref_table[n] = ref_table[first];        /* duplicate, copy the reference */
    }
    else
    {
        switch (getObjectType(ptr, n))
        {
            case VX_TYPE_IMAGE:         status = importImage(context, ptr, ref_table, n);       break;
            case VX_TYPE_LUT:           status = importLUT(context, ptr, ref_table, n);         break;
            case VX_TYPE_DISTRIBUTION:  status = importDistribution(context, ptr, ref_table, n);break;
            case VX_TYPE_THRESHOLD:     status = importThreshold(context, ptr, ref_table, n);   break;
            case VX_TYPE_MATRIX:        status = importMatrix(context, ptr, ref_table, n);      break;
            case VX_TYPE_CONVOLUTION:   status = importConvolution(context, ptr, ref_table, n); break;
            case VX_TYPE_SCALAR:        status = importScalar(context, ptr, ref_table, n);      break;
            case VX_TYPE_ARRAY:         status = importArray(context, ptr, ref_table, n);       break;
            case VX_TYPE_REMAP:         status = importRemap(context, ptr, ref_table, n);       break;
            case VX_TYPE_OBJECT_ARRAY:  status = importObjectArray(context, ptr, ref_table, n, VX_TYPE_INVALID); break;
            case VX_TYPE_PYRAMID:       status = importPyramid(context, ptr, ref_table, n);     break;
            case VX_TYPE_DELAY:         status = importDelay(context, ptr, ref_table, n);       break;
            case VX_TYPE_TENSOR:        status = importTensor(context, ptr, ref_table, n);      break;
            case VX_TYPE_KERNEL:        status = importKernel(context, ptr, ref_table, n);      break;
            default: /* something we don't know how to import - graphs have been created already */
                status = VX_ERROR_NOT_SUPPORTED;
        }
        if (VX_SUCCESS == status && ref_table[n])
        {
            status = vxSetReferenceName(ref_table[n], getObjectName(ptr, n));
        }
    }
    return status;
}

static vx_status createGraphObjects(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 i;
    DEBUGPRINTF("Creating graph objects\n");
    for (i = 0; i < getHeader(ptr)->actual_numrefs && VX_SUCCESS == status; ++i)
    {
        if (VX_TYPE_GRAPH == getObjectType(ptr, i))
        {
            vx_uint32 first = getFirstIndex(ptr, i);
            /* Check that this is not a duplicate */
            if (first < i)
            {
                ref_table[i] = ref_table[first];    /* just copy the reference */
            }
            else
            {
                ref_table[i] = (vx_reference)vxCreateGraph(context);
                status = vxGetStatus(ref_table[i]);
                if (VX_SUCCESS == status)
                {
                    status = vxSetReferenceName(ref_table[i], getObjectName(ptr, i));
                }
            }
        }
    }
    return status;
}

static vx_status importObjectsOfType(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table, vx_enum vx_type)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 i;
    for (i = 0; i < getHeader(ptr)->actual_numrefs && VX_SUCCESS == status; ++i)
    {
        /* Only process objects we haven't already processed */
        if (NULL == ref_table[i] &&
            (VX_TYPE_INVALID == vx_type || getObjectType(ptr, i) == vx_type)
           )
        {
            status = importObject(context, ptr, ref_table, i);
        }
    }
    return status;
}

static vx_status importObjectArraysFromTensor(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 i;
    for (i = 0; i < getHeader(ptr)->actual_numrefs && VX_SUCCESS == status; ++i)
    {
        /* Only process objects we haven't already processed */
        if (NULL == ref_table[i] &&
            (VX_TYPE_OBJECT_ARRAY == getObjectType(ptr, i))
           )
        {
            status = importObjectArray(context, ptr, ref_table, i, VX_TYPE_TENSOR);
        }
    }
    return status;
}

static vx_status buildGraphs(vx_context context, const vx_uint8 *ptr, vx_reference *ref_table)
{
    /* All data objects have been created; ref_table is (or should be) full */

    vx_status status = VX_SUCCESS;
    vx_uint32 i;
    DEBUGPRINTF("Building graphs\n");
    for (i = 0; i < getHeader(ptr)->actual_numrefs && VX_SUCCESS == status; ++i)
    {
        /* Format of data:
            vx_uint32 num_delays
            vx_uint32 delays[num_delays]        # indices into offsets table 
            vx_uint32 num_nodes
            struct
                vx_uint32 kernel_index          # index into offsets table for the kernel 
                vx_uint32 params[num_kernel_params]    # num_kernel_params from the kernel entry
                                                # index into offsets table for each parameter;
                                                # 0xFFFFFFFF is used for absent optional parameters 
                vx_uint8 is_replicated flag
                case is_replicated of
                0:                              # no data 
                
                default:                        # non-zero or true value 
                    vx_uint8 replicated flags[num_params]
                end_case 
                vx_uint32 affinity              # note sample implementation has illegal zero affinity 
                vx_border_t border mode
            end_struct nodes[num_nodes]
            vx_uint32 num_graph_params
            struct
                vx_uint32 node_reference
                vx_uint32 parameter_index
            end_struct graph_params[num_graph_params]
        */
        if (VX_TYPE_GRAPH == getObjectType(ptr, i))
        {
            vx_uint32 first = getFirstIndex(ptr, i);
            /* Check that this is not a duplicate  - we can only build graphs once! */
            if (first == i)
            {
                VXBinObjectHeader header;
                status = importObjectHeader(ptr, &header, i);
                vx_graph g = (vx_graph)ref_table[i];
                vx_uint32 num_delays;
                vx_uint32 num_nodes;
                vx_uint32 c;
                /* register any delays for auto-aging */
                importVxUint32(&header, &num_delays);
                for (c = 0; c < num_delays && VX_SUCCESS == status; ++c)
                {
                    vx_uint32 delay_ix;
                    importVxUint32(&header, &delay_ix);
                    if (checkIndex(ptr, delay_ix))
                    {
                        status = vxRegisterAutoAging(g, (vx_delay)ref_table[delay_ix]);
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                    }
                    if (status)
                    {
                        DEBUGPRINTF("Failed to register delay %u for auto-aging\n", c);
                    }
                }
                importVxUint32(&header, &num_nodes);
                DEBUGPRINTF("Building graph of %u nodes\n", num_nodes);
                /* create nodes and add parameters */
                for (c = 0; c < num_nodes && VX_SUCCESS == status; ++c)
                {
                    vx_uint32 kernel_ix;
                    vx_uint32 num_params;
                    vx_uint8 is_replicated;
                    vx_uint32 p;
                    vx_node node;
                    importVxUint32(&header, &kernel_ix);
                    if (checkIndex(ptr, kernel_ix))
                    {
                        vx_kernel kernel = (vx_kernel)ref_table[kernel_ix];
                        node = vxCreateGenericNode(g, kernel);
                        num_params = kernel->signature.num_parameters;
                        for (p = 0; p < num_params && VX_SUCCESS == status; ++p)
                        {
                            vx_uint32 param_ix;
                            importVxUint32(&header, &param_ix);
                            if (0xFFFFFFFF == param_ix)     /* Optional parameter */
                                status = vxSetParameterByIndex(node, p, NULL);
                            else if (checkIndex(ptr, param_ix))
                                status = vxSetParameterByIndex(node, p, ref_table[param_ix]);
                            else
                                status = VX_ERROR_INVALID_PARAMETERS;
                            if (status)
                            {
                                DEBUGPRINTF("Failed to set parameter %u of node %u\n", p, c);
                            }
                        }
                        importVxUint8(&header, &is_replicated);
                        if (is_replicated && VX_SUCCESS == status)
                        {
                            vx_uint8 rep_flag;
                            /* node is replicated, get replication flags */
                            vx_bool flags[VX_INT_MAX_PARAMS];
                            for (p = 0; p < num_params; ++p)
                            {
                                importVxUint8(&header, &rep_flag);
                                flags[p] = (rep_flag) ? vx_true_e : vx_false_e;
                            }
                            status = vxReplicateNode(g, node, flags, num_params);
                            if (status)
                            {
                                DEBUGPRINTF("Failed to replicate node %u\n", c);
                            }
                        }
                        /* set affinity */
                        if (VX_SUCCESS == status)
                        {
                            vx_enum affinity;
                            importVxEnum(&header, &affinity);
                            node->affinity = affinity;
                        }
                        /* set border mode */
                        if (VX_SUCCESS == status)
                        {
                            vx_border_t border;
                            importVxBorderT(&header, &border);
                            status = vxSetNodeAttribute(node, VX_NODE_BORDER, &border, sizeof(vx_border_t));
                            if (status)
                            {
                                DEBUGPRINTF("Failed to set border for node %u\n", c);
                            }
                        }
                        vxReleaseNode(&node);
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                    }
                }
                /* Finally, set up the graph parameters.
                 */
                if (VX_SUCCESS == status)
                {
                    importVxUint32(&header, &g->numParams);
                    DEBUGPRINTF("Graph with %u parameters\n", g->numParams);
                    for (c = 0; c < g->numParams; ++c)
                    {
                        vx_uint32 node_ix;
                        importVxUint32(&header, &node_ix);
                        if (node_ix < num_nodes)
                            g->parameters[c].node = g->nodes[node_ix];
                        else
                            status = VX_ERROR_INVALID_PARAMETERS;
                        importVxUint32(&header, &(g->parameters[c].index));
                    }
                }              
                if (NULL == header.curptr)
                    status = VX_FAILURE;
            }
            else
            {
                DEBUGPRINTF("found duplicate graph: indices %u and %u\n", first, i);
            }
        }
    }
    /* Now verify all the graphs and create any graph kernels
       In this sample implementation verification is necessary, but for other implementations
       we hope that in fact we have an immutable graph, and here we just call validation
       and initialisation functions of any user nodes.
     */
    for (i = 0; i < getHeader(ptr)->actual_numrefs && VX_SUCCESS == status; ++i)
    {
        if (VX_TYPE_GRAPH == getObjectType(ptr, i))
        {
            vx_uint32 first = getFirstIndex(ptr, i);
            /* Check that this is not a duplicate  - we can only build graphs once! */
            if (first == i)
            {
                VXBinObjectHeader header;
                importObjectHeader(ptr, &header, i);
                vx_graph g = (vx_graph)ref_table[i];
                if (VX_IX_USE_IMPORT_AS_KERNEL == header.use)
                {
                    /* Bugzilla 16207: create a user kernel that calls this graph.
                    The kernel takes the same name as the graph, and the kernel replaces
                    the graph in the reference table. See createGraphKernel() for details
                    of operation.
                    */
                    ref_table[i] = (vx_reference)createGraphKernel(context, g);
                    status = vxGetStatus(ref_table[i]);
                }
                if (VX_SUCCESS == status)
                {
                    status = vxVerifyGraph(g);
                    if (status)
                    {
                        DEBUGPRINTF("Failed to verify graph\n");
                    }
                }
                else
                {
                    DEBUGPRINTF("Failed to create graph kernel\n");
                }
            }
        }
    }
    return status;
}

VX_API_ENTRY vx_import VX_API_CALL vxImportObjectsFromMemory(
    vx_context context,  
    vx_size numrefs,     
    vx_reference *refs,  
    const vx_enum * uses,
    const vx_uint8 * ptr,
    vx_size length)
{
    vx_uint32 checksum = 0;
    vx_import import = NULL;
    vx_status status = VX_SUCCESS;
    /* Perform initial checks */
    const VXBinHeader *header = getHeader(ptr);
    const vx_uint8 *curptr = ptr;
    for (curptr = ptr; curptr < ptr + header->length - sizeof(checksum); ++curptr)
    {
        checksum += *curptr;
    }
    if ((VX_IX_VERSION == header->version && checksum != *(vx_uint32 *)curptr) ||   /* only check checksum if version match */
        VX_SUCCESS != vxGetStatus((vx_reference)context) ||
        NULL == refs ||
        NULL == uses ||
        NULL == ptr ||
        length <= sizeof(VXBinHeader) ||
        VX_IX_ID != header->vx_ix_id ||
        VX_IX_VERSION < header->version ||      /* Should always be backwards compatible */
        length != header->length ||
        numrefs != header->numrefs ||
        header->actual_numrefs < header->numrefs ||
        length <= sizeof(VXBinHeader) + 
            numrefs * sizeof(vx_enum) + 
            header->actual_numrefs * sizeof(vx_uint64)
        )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        DEBUGPRINTF("Initial checks failed\n");
    }
    else /* Check that uses match and that refs is suitably populated with valid references where required */
    {
        vx_uint32 i;
        const vx_enum * header_uses = getUses(ptr);
        for (i = 0; i < numrefs && NULL == import; ++i)
        {
            if (uses[i] != header_uses[i] || (VX_IX_USE_APPLICATION_CREATE == uses[i] && VX_SUCCESS != vxGetStatus(refs[i])))
            {
                DEBUGPRINTF("Uses don't match or refs invalid\n");
                status = VX_ERROR_INVALID_PARAMETERS;
            }
        }       
    }
    if (VX_SUCCESS == status)
    {
        vx_uint32 i;
        vx_reference *ref_table = (vx_reference *)calloc(sizeof(vx_reference), header->actual_numrefs);
        if (ref_table)
        {
            for (i = 0; i < numrefs; ++i)
            {
                if (VX_IX_USE_APPLICATION_CREATE != uses[i])
                    ref_table[i] = NULL;
                else
                    ref_table[i] = refs[i];
            }
            status = createGraphObjects(context, ptr, ref_table);   /* Create graph objects */
        }
        else
            status = VX_ERROR_NO_MEMORY;
        /* Import all data objects.
           The order is important, since we need to create parents first; these will in
           turn create their children.
        */
        if (VX_SUCCESS == status)
            status = importObjectsOfType(context, ptr, ref_table, VX_TYPE_DELAY);
        if (VX_SUCCESS == status)
            status = importObjectsOfType(context, ptr, ref_table, VX_TYPE_OBJECT_ARRAY);
        if (VX_SUCCESS == status)
            status = importObjectArraysFromTensor(context, ptr, ref_table);
        if (VX_SUCCESS == status)
            status = importObjectsOfType(context, ptr, ref_table, VX_TYPE_PYRAMID);
        /* Now we do the rest, by specifying VX_TYPE_INVALID */
        if (VX_SUCCESS == status)
            status = importObjectsOfType(context, ptr, ref_table, VX_TYPE_INVALID);
        if (VX_SUCCESS == status)
            status = buildGraphs(context, ptr, ref_table);              /* Build and verify graphs */
        if (VX_SUCCESS == status)
        {
            /* Now we need to clean up kernels that are private to this import. These are
            graph kernels not listed, with use VX_IX_USE_EXPORT_VALES.
            We simply set their names to zero length, which ensures that they can't be
            found by name and so interfere with other imports */
            for (i = numrefs + 1; i < header->actual_numrefs; ++i)
            {
                if (VX_TYPE_KERNEL == ref_table[i]->type &&
                    ((vx_kernel)ref_table[i])->user_kernel &&
                    VX_TYPE_GRAPH == ref_table[i]->scope->type)
                {
                    VXBinObjectHeader header;
                    importObjectHeader(ptr, &header, i);
                    if (VX_IX_USE_EXPORT_VALUES == header.use)
                        ((vx_kernel)ref_table[i])->name[0] = 0;
                }
            }
        }
        if (VX_SUCCESS == status)
        {
            vx_uint32 i;
            import = ownCreateImportInt(context, VX_IMPORT_TYPE_BINARY, numrefs);
            if (import && VX_TYPE_IMPORT == import->base.type && import->refs)
            {
                for (i = 0; i < numrefs; ++i)
                {
                    refs[i] = ref_table[i];
                    ownIncrementReference(refs[i], VX_INTERNAL);
                    import->refs[i] = refs[i];
                }
                /* Now clean up other references */
                for (i=numrefs; i < header->actual_numrefs; ++i)
                    if (ref_table[i]->external_count)
                        vxReleaseReference(&ref_table[i]);
            }
        }
        free(ref_table);
    }
    if (VX_SUCCESS != status)
    {
        import = (vx_import)ownGetErrorObject(context, status);
    }
    return import;
}

#endif

