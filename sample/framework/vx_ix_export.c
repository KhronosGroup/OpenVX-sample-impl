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
#include <stdio.h>
#include <VX/vx_khr_nn.h>
#ifndef VX_IX_USE_IMPORT_AS_KERNEL
#define VX_IX_USE_IMPORT_AS_KERNEL (VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_IX_USE) + 0x3) /*!< \brief Graph exported as user kernel. */
#endif
#define DEBUGPRINTF(x,...)
//#define DEBUGPRINTF printf

/*!
 * \file
 * \brief The Export Object as Binary API.
 *  See vx_ix_format.txt for details of the export format
 */
 
/*
Export objects to binary (import is in a separate file vx_ix_import.c)

Some notes on operation not fully characterised by the specification:
numrefs == 0 is tolerated
duplicate references are tolerated and all but the first ignored.
conflicts in the 'uses' array values are tolerated and resolved in what I think is a sensible way.
No real attempt has been made to reduce the size (or speed) of the export - this is a pragmatic implementation
that seeks merely to satisfy the specification.

*IMPORTANT NOTE*
================
This implementation requires that the exporting and importing implementations have the same endianness.

*/
#define VX_IX_ID (0x1234C0DE)           /* To identify an export and make sure endian matches etc. */
#define VX_IX_VERSION ((VX_VERSION << 8) + 1)

struct VXBinHeaderS {                   /* What should be at the start of the binary blob */
    vx_uint32 vx_ix_id;                 /* Unique ID */
    vx_uint32 version;                  /* Version of the export/import */
    vx_uint64 length;                   /* Total length of the blob */
    vx_uint32 numrefs;                  /* Number of references originally supplied */
    vx_uint32 actual_numrefs;           /* Actual number of references exported */
};
typedef struct VXBinHeaderS VXBinHeader;

#define USES_OFFSET (sizeof(VXBinHeader))/* Where the variable part of the blob starts */

#define MAX_CONCURRENT_EXPORTS (10)     /* Arbitrary limit on number of exports allowed at any one time */
#define IMAGE_SUB_TYPE_NORMAL (0)       /* Code to indicate 'normal' image */
#define IMAGE_SUB_TYPE_UNIFORM (1)      /* Code to indicate uniform image */
#define IMAGE_SUB_TYPE_ROI (2)          /* Code to indocate ROI image */
#define IMAGE_SUB_TYPE_CHANNEL (3)      /* Code to indicate image created from channel */
#define IMAGE_SUB_TYPE_TENSOR (4)       /* Code to indicate an image created from a tensor */
#define IMAGE_SUB_TYPE_HANDLE (5)       /* Code to indicate an image created from handle */

#define OBJECT_UNPROCESSED (0)          /* Object has not yet been processed */
#define OBJECT_UNSIZED (1)              /* Object has not yet been sized */
#define OBJECT_NOT_EXPORTED (2)         /* Object is sized but not exported */
#define OBJECT_EXPORTED (3)             /* Object has already been exported */
#define OBJECT_DUPLICATE (4)            /* Object is a duplicate supplied by user */

struct VXBinExportRefTableS {
    vx_reference ref;                   /* The reference */
    vx_enum status;                     /* Progress of export */
    vx_enum use;                        /* Use for the reference */
};

typedef struct VXBinExportRefTableS VXBinExportRefTable;
struct VXBinExportS {
    vx_context context;                 	/* Context supplied by the application */
    const vx_reference *refs;       	/* Refs list supplied by the application */
    const vx_enum *uses;            	/* Uses supplied by the application */
    VXBinExportRefTable *ref_table;   /* Table for all references compiled by export function */
    vx_size numrefs;                    	/* Number of references given by the application */
    vx_size actual_numrefs;             	/* Actual number of references to be exported */
    vx_uint8 *export_buffer;            	/* Buffer allocated for export */
    vx_uint8 *curptr;                   	/* Pointer into buffer for next free location */
    vx_size export_length;              	/* Size in bytes of the buffer */
};
typedef struct VXBinExportS VXBinExport;

static vx_uint8 *exports[MAX_CONCURRENT_EXPORTS] = {NULL};
static vx_uint32 num_exports = 0;

static int exportToProcess(VXBinExport *xport, vx_size n, int calcSize)
{
    return (calcSize ? OBJECT_UNSIZED : OBJECT_NOT_EXPORTED) == xport->ref_table[n].status;
}

static int isFromHandle(vx_reference ref)
{
    /* Return TRUE if the reference has been created from a handle (i.e., by the application)*/
    return (VX_TYPE_IMAGE == ref->type && VX_MEMORY_TYPE_NONE != ((vx_image)ref)->memory_type);
}

static int isImmutable(vx_kernel k, vx_uint32 p)
{
    int retval = 0;
    /* return true if parameter p of kernel k is immutable
       This is based upon the parameter to a node function being not an OpenVX object
    */
    if (k->enumeration < VX_KERNEL_MAX_1_2 && k->enumeration >= VX_KERNEL_COLOR_CONVERT)
    {
        switch (k->enumeration)
        {
            case VX_KERNEL_NON_LINEAR_FILTER:
            case VX_KERNEL_SCALAR_OPERATION:
                if (0 == p)
                    retval = 1;
                break;
            case VX_KERNEL_CHANNEL_EXTRACT:
            case VX_KERNEL_HOUGH_LINES_P:
            case VX_KERNEL_TENSOR_CONVERT_DEPTH:
                if (1 == p)
                    retval = 1;
                break;
            case VX_KERNEL_LBP:
                if (1 == p || 2 == p)
                    retval = 0;
                break;
            case VX_KERNEL_SCALE_IMAGE:
            case VX_KERNEL_ADD:
            case VX_KERNEL_SUBTRACT:
            case VX_KERNEL_CONVERTDEPTH:
            case VX_KERNEL_WARP_AFFINE:
            case VX_KERNEL_WARP_PERSPECTIVE:
            case VX_KERNEL_FAST_CORNERS:
            case VX_KERNEL_REMAP:
            case VX_KERNEL_HALFSCALE_GAUSSIAN:
            case VX_KERNEL_NON_MAX_SUPPRESSION:
            case VX_KERNEL_MATCH_TEMPLATE:
            case VX_KERNEL_TENSOR_ADD:
            case VX_KERNEL_TENSOR_SUBTRACT:
                if (2 == p)
                    retval = 1;
                break;
            case VX_KERNEL_CANNY_EDGE_DETECTOR:
            case VX_KERNEL_TENSOR_TRANSPOSE:
                if (2 == p || 3 == p)
                    retval = 1;
                break;
            case VX_KERNEL_TENSOR_MATRIX_MULTIPLY:
                if (3 == p)
                    retval = 1;
                break;
            case VX_KERNEL_MULTIPLY:
            case VX_KERNEL_HOG_FEATURES:
            case VX_KERNEL_TENSOR_MULTIPLY:
                if (3 == p || 4 == p)
                    retval = 1;
                break;
            case VX_KERNEL_HARRIS_CORNERS:
                if (4 == p || 5 == p)
                    retval = 1;
                break;
            case VX_KERNEL_OPTICAL_FLOW_PYR_LK:
                if (5 == p || 9 == p)
                    retval = 1;
                break;
            case VX_KERNEL_HOG_CELLS:
                if (1 == p || 2 == p || 3 == p)
                    retval = 1;
                break;
            case VX_KERNEL_BILATERAL_FILTER:
                if (1 == p || 2 == p || 3 == p)
                    retval = 1;
                break;
            default: ;
        }
    }
    return retval;
}

static void malloc_export(VXBinExport *xport)
{
    xport->export_buffer = NULL;
    /* allocate memory for an export */
    if (xport->export_length && num_exports < MAX_CONCURRENT_EXPORTS)
    {
        for (int i = 0; i < MAX_CONCURRENT_EXPORTS && NULL == xport->export_buffer; ++i)
            if (NULL == exports[i])
            {
                xport->export_buffer = (vx_uint8 *)malloc(xport->export_length);
                exports[i] = xport->export_buffer;
            }
    }
    /* increment number of exports if we have been successful */
    if (xport->export_buffer)
        ++num_exports;
}

static vx_size indexIn(const VXBinExportRefTable *ref_table, const vx_reference ref, const vx_size num)
{
    /* Return the index of the reference in the table, or num if it is not there */
    vx_size i;
    for (i = 0; i < num && ref != ref_table[i].ref; ++i)
    {
        ;
    }
    return i;
}

static vx_uint32 indexOf(const VXBinExport *xport, const vx_reference ref)
{
    /* Return the first index of the reference in the table or (xport->actual_numrefs + ref->type) if it is not there.
       Note that it's not an error if the reference is not found; it may be that an object is being exported without
       its parent, for example a pyramid level or element of an object array, or that we are looking for the context.
       We test for context specifically to make the search faster, as this could be a frequent case. The type of an
       unfound reference (usually scope) is encoded for error-checking upon import.
    We also return 0xffffffff if the reference was NULL; this is for optional parameter checking.
    */
    vx_uint32 i = xport->actual_numrefs + VX_TYPE_CONTEXT;
    if (ref)    /* Check for NULL: this could be an optional parameter */
    {
        if (VX_TYPE_CONTEXT != ref->type)
        {
            for (i = 0; i < xport->actual_numrefs && ref != xport->ref_table[i].ref; ++i)
            {
                ;
            }
            if (i == xport->actual_numrefs)     /* not found; encode the type of the reference for future use on import */
                i += ref->type;
        }
    }
    else
    {
        i = 0xFFFFFFFF;
    }
    return i;
}

static vx_uint8 *addressOf(VXBinExport *xport, const vx_size n)
{
    /* Return the address of the offset into the blob of a given reference by it's index. */
    return xport->export_buffer + USES_OFFSET + xport->numrefs * sizeof(vx_uint32) + n * sizeof(vx_uint64);
}

static void calculateUses(VXBinExport *xport)
{
	/* Calculate what the uses for each object should be. This is essentially patching up uses assigned
    by putInTable...
	*/
	vx_size i;
	for (i = xport->numrefs; i < xport->actual_numrefs; ++i)
    {
        switch (xport->ref_table[i].use)
        {
            case VX_IX_USE_APPLICATION_CREATE:
            case VX_IX_USE_EXPORT_VALUES:
            case VX_IX_USE_NO_EXPORT_VALUES:
            case VX_IX_USE_IMPORT_AS_KERNEL:
                break;          /* Already assigned, do nothing */
            case VX_OUTPUT:     /* Part processed by putInTable, is a net output */
                xport->ref_table[i].use = VX_IX_USE_NO_EXPORT_VALUES;
            default:            /* All other cases, assume we need to export values unless virtual */
                xport->ref_table[i].use = xport->ref_table[i].ref->is_virtual ?
                                            VX_IX_USE_NO_EXPORT_VALUES :
                                            VX_IX_USE_EXPORT_VALUES;
                break;
        }
    }
}

static vx_size putInTable(VXBinExportRefTable *ref_table, const vx_reference newref, vx_size actual_numrefs, vx_enum use)
{
    /* If the reference newref is NULL or is already in the table, just return actual_numrefs, else put
    it in the table and return actual_numrefs + 1. Note that NULL can occur for absent optional parameters
    and is treated specially during output and so does not appear in the table since it can't have been
    supplied in original list of references passed to the export function as they must pass getStatus(). */
    if (newref)
    {
        vx_size index = indexIn(ref_table, newref, actual_numrefs);
        if (index == actual_numrefs)
        {
            /* Must be a new reference, initialise the table entry */
            ref_table[index].ref = newref;
            ref_table[index].status = OBJECT_UNPROCESSED;
            ref_table[index].use = vx_true_e == newref->is_virtual ? VX_IX_USE_NO_EXPORT_VALUES : use;
            ++actual_numrefs;
        }
        else
        {
            /* Adjust use if required. This allows for inheriting of APPLICATION_CREATE and
            EXPORT_VALUES, as well as adjusting uses according to readers and writers. */
            if (VX_INPUT == ref_table[index].use ||
                VX_BIDIRECTIONAL == ref_table[index].use ||
                VX_IX_USE_APPLICATION_CREATE == use ||
                VX_IX_USE_EXPORT_VALUES == use)
            {
                ref_table[index].use = use;
            }
        }
        if (OBJECT_UNPROCESSED == ref_table[index].status)
        {   /* We make sure we are not processing something we have already processed,
            otherwise we could get in a loop because of processing parents that have
            children poitning to their parents...*/
            
            ref_table[index].status = OBJECT_UNSIZED;
            
            /* Now we have to expand compound objects and ROI images */
            if (VX_TYPE_GRAPH == newref->type)
            {
                 /* Need to enumerate all graph parameters, nodes and node parameters in a graph */
                vx_uint32 ix;               /* for enumerating graph parameters, nodes and delays*/
                vx_graph g = (vx_graph)newref;
                for (ix = 0; ix < g->numParams; ++ix)
                {
                    /* Graph parameters - bugzilla 16313 */
                    vx_reference r =  g->parameters[ix].node->parameters[g->parameters[ix].index];
                    actual_numrefs = putInTable(ref_table, r, actual_numrefs, VX_IX_USE_NO_EXPORT_VALUES);
                }
                for (ix = 0; ix < g->numNodes; ++ix)
                {
                    vx_node n = g->nodes[ix];
                    vx_uint32 p;        /* for enumerating node parameters */
                    /* We don't have to put in nodes are these are output as part of each graph */
                    /* Bugzilla 16337 - we need to add in the kernel object, however. */
                    /* Bugzilla 16207 - may have to export kernel as VX_IX_USE_EXPORT_VALUES rather than VX_IX_USE_APPLICATION_CREATE */
                    vx_enum kernel_use = VX_IX_USE_NO_EXPORT_VALUES;
                    if (VX_IX_USE_NO_EXPORT_VALUES != ref_table[index].use &&
                        n->kernel->user_kernel &&
                        VX_TYPE_GRAPH == n->kernel->base.scope->type)
                    {
                        kernel_use = VX_IX_USE_EXPORT_VALUES;
                    }
                    actual_numrefs = putInTable(ref_table, (vx_reference)n->kernel, actual_numrefs, kernel_use);
                    for (p = 0; p < n->kernel->signature.num_parameters; ++p)
                    { 
                        /* Add in each parameter. Note that absent optional parameters (NULL) will not end
                        up in the table and have special treatment on output (and input). */
                        actual_numrefs = putInTable(ref_table, n->parameters[p], actual_numrefs, n->kernel->signature.directions[p]);
                        /* If a node is replicated, need to check replicated flags and put parent in the list as well */
                        if (n->is_replicated && n->replicated_flags[p])
                        {
                            actual_numrefs = putInTable(ref_table, n->parameters[p]->scope, actual_numrefs, n->kernel->signature.directions[p]);
                        }
                    }
                }
                /* Need to enumerate all delays that are registered with this graph */
                for (ix = 0; ix < VX_INT_MAX_REF; ++ix)
                {
                    actual_numrefs = putInTable(ref_table, (vx_reference)g->delays[ix], actual_numrefs, VX_IX_USE_NO_EXPORT_VALUES);
                }
                /* Now need to patch up uses; any objects read but not written become VX_IX_EXPORT_VALUES */
                for (ix = 1; ix < actual_numrefs; ++ix)
                {
                    if (VX_INPUT == ref_table[ix].use ||
                        VX_BIDIRECTIONAL == ref_table[ix].use)
                    {
                        ref_table[ix].use = VX_IX_USE_EXPORT_VALUES;
                    }
                }
            }
            else if (VX_TYPE_KERNEL == newref->type && VX_IX_USE_EXPORT_VALUES == ref_table[index].use)
            {
                /* Bugzilla 16207 */
                actual_numrefs = putInTable(ref_table, newref->scope, actual_numrefs, VX_IX_USE_EXPORT_VALUES);
                if (VX_TYPE_GRAPH != newref->scope->type)
                {
                    DEBUGPRINTF("Error in graph kernel export; scope of kernel is not a graph! Difference is %d\n", newref->scope->type - VX_TYPE_GRAPH);
                }
                else
                {
                    DEBUGPRINTF("Requesting export of graph with %u parameters\n", ((vx_graph)(newref->scope))->numParams);
                }
            }
            else if (VX_TYPE_DELAY == newref->type)
            {
                vx_delay delay = (vx_delay)newref;
                vx_uint32 c;
                for (c = 0; c < delay->count; ++c)
                {
                    /* if parent use is APPLICATION_CREATE or EXPORT_VALUES sub-object inherits use */
                    actual_numrefs = putInTable(ref_table, delay->refs[c], actual_numrefs, ref_table[index].use);
                }
            }
            else if (VX_TYPE_PYRAMID == newref->type && vx_false_e == newref->is_virtual)
            {
                vx_pyramid pyramid = (vx_pyramid)newref;
                vx_uint32 c;
                for (c = 0; c < pyramid->numLevels; ++c)
                {
                    /* if parent use is APPLICATION_CREATE or EXPORT_VALUES sub-object inherits use */
                    actual_numrefs = putInTable(ref_table, (vx_reference)pyramid->levels[c], actual_numrefs, ref_table[index].use);
                }
            }
            else if (VX_TYPE_OBJECT_ARRAY == newref->type)
            {
                vx_object_array objArray = (vx_object_array)newref;
                vx_uint32 c;
                for (c = 0; c < objArray->num_items; ++c)
                {
                    /* if parent use is APPLICATION_CREATE or EXPORT_VALUES sub-object inherits use */
                    actual_numrefs = putInTable(ref_table, objArray->items[c], actual_numrefs, ref_table[index].use);
                }
            }
            else if (VX_TYPE_IMAGE == newref->scope->type
                        || VX_TYPE_TENSOR == newref->scope->type
                    )
            {
                /* Must be an ROI or image from channel, or a tensor from view
                so put parent object in the table as well; use becomes the same as child */
                actual_numrefs = putInTable(ref_table, newref->scope, actual_numrefs, ref_table[index].use);
            }
            /* Special case of an object array of images created from tensor. Here we
            need to put both the object array and the tensor in the table */
            else if (VX_TYPE_OBJECT_ARRAY == newref->scope->type &&
                     VX_TYPE_IMAGE == newref->type &&
                     ((vx_image)newref)->parent &&
                     VX_TYPE_TENSOR == ((vx_image)newref)->parent->base.type)
            {
                actual_numrefs = putInTable(ref_table, newref->scope, actual_numrefs, ref_table[index].use);
                actual_numrefs = putInTable(ref_table, (vx_reference)(((vx_image)newref)->parent), actual_numrefs, ref_table[index].use);
            }
        }
    }
    return actual_numrefs;
}

static vx_status exportBytes(VXBinExport *xport, const void *data, const vx_size length, int calcSize)
{
	vx_status status = VX_SUCCESS;
    if (calcSize)
    {
        xport->export_length += length;
    }
    else if (xport->export_length - (xport->curptr - xport->export_buffer) >= length)
    {
        memcpy(xport->curptr, data, length);
        xport->curptr += length;
    }
    else
    {
        DEBUGPRINTF("Failure in export bytes\n");
        status = VX_FAILURE;
    }
	return status;
}

#define EXPORT_FUNCTION(funcname, datatype) \
static vx_status funcname(VXBinExport *xport, const datatype data, int calcSize) \
{ \
    return exportBytes(xport, &data, sizeof(data), calcSize); \
}

#define EXPORT_FUNCTION_PTR(funcname, datatype) \
static vx_status funcname(VXBinExport *xport, const datatype *data, int calcSize) \
{ \
    return exportBytes(xport, data, sizeof(*data), calcSize); \
}

EXPORT_FUNCTION(exportVxUint64, vx_uint64)
/* EXPORT_FUNCTION(exportVxInt64, vx_int64) */
/* EXPORT_FUNCTION(exportVxFloat64, vx_float64) */
EXPORT_FUNCTION(exportVxUint32, vx_uint32)
EXPORT_FUNCTION(exportVxInt32, vx_int32)
EXPORT_FUNCTION(exportVxFloat32, vx_float32)
EXPORT_FUNCTION(exportVxEnum, vx_enum)
EXPORT_FUNCTION(exportVxDfImage, vx_df_image)
EXPORT_FUNCTION(exportVxUint16, vx_uint16)
EXPORT_FUNCTION(exportVxInt16, vx_int16)
EXPORT_FUNCTION(exportVxUint8, vx_uint8)
EXPORT_FUNCTION_PTR(exportVxBorderT, vx_border_t)
EXPORT_FUNCTION_PTR(exportVxCoordinates2dT, vx_coordinates2d_t)

static vx_status exportHeader(VXBinExport *xport, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_size i;
    if (!calcSize && ((NULL == xport->export_buffer) || xport->actual_numrefs & 0xffffffff00000000U))
    {
        DEBUGPRINTF("No memory\n");
        status = VX_ERROR_NO_MEMORY;
    }
    else
    {
        status |= exportVxUint32(xport, (vx_uint32)VX_IX_ID, calcSize);
        status |= exportVxUint32(xport, (vx_uint32)VX_IX_VERSION, calcSize);
        status |= exportVxUint64(xport, (vx_uint64)xport->export_length, calcSize);
        status |= exportVxUint32(xport, (vx_uint32)xport->numrefs, calcSize);
        status |= exportVxUint32(xport, (vx_uint32)xport->actual_numrefs, calcSize);
        for (i = 0; i < xport->numrefs && VX_SUCCESS == status; ++i)
        {
            status = exportVxUint32(xport, (vx_uint32)xport->uses[i], calcSize);
        }
        if (calcSize)
        {
            xport->export_length += xport->actual_numrefs * sizeof(vx_uint64);
        }
        else if (xport->export_length - (xport->curptr - xport->export_buffer) >= xport->actual_numrefs * sizeof(vx_uint64))
        {
            xport->curptr += xport->actual_numrefs * sizeof(vx_uint64);
        }
        else
        {
            DEBUGPRINTF("Failed in export header\n");
            status = VX_FAILURE;
        }
    }
    return status;
}

static vx_status exportObjectHeader(VXBinExport *xport, vx_size n, int calcSize)
{
    /* Set the index into the memory blob for entry ix to be the current pointer, and
       then exports the common information (header) for an object */
    vx_status status = VX_SUCCESS;
    if (calcSize)
    {
        xport->export_length +=         /* Check below that what is exported matches */
            sizeof(vx_enum) +           /* type of this object */
            sizeof(vx_enum) +           /* use for this object */
            VX_MAX_REFERENCE_NAME +     /* name */
            sizeof(vx_uint8) +          /* virtual flag */
            sizeof(vx_uint32);          /* scope */
        xport->ref_table[n].status = OBJECT_NOT_EXPORTED;
    }
    else
    {
        VXBinExportRefTable *rt = xport->ref_table + n;
        vx_uint8 *p = xport->curptr;
        xport->curptr = addressOf(xport, n);
        status = exportVxUint64(xport, (vx_uint64)(p - xport->export_buffer), 0);
        xport->curptr = p;
        status |= exportVxEnum(xport, rt->ref->type, 0);                                /* Export type of this object */
        status |= exportVxEnum(xport, rt->use, 0);                                      /* Export usage of this object */
        status |= exportBytes(xport, rt->ref->name, VX_MAX_REFERENCE_NAME, 0);          /* Export name of this object */
        status |= exportVxUint8(xport, (vx_uint8)(rt->ref->is_virtual ? 1 : 0), 0);     /* Export virtual flag */
        status |= exportVxUint32(xport, indexOf(xport, rt->ref->scope), 0);             /* Export scope, for virtuals etc */
        xport->ref_table[n].status = OBJECT_EXPORTED;
    }
    return status;
}


static void computeROIStartXY(vx_image parent, vx_image roi, vx_uint32 *x, vx_uint32 *y)
{
    vx_uint32 offset = roi->memory.ptrs[0] - parent->memory.ptrs[0];
    *y = offset * roi->scale[0][VX_DIM_Y] / roi->memory.strides[0][VX_DIM_Y];
    *x = (offset - ((*y * roi->memory.strides[0][VX_DIM_Y]) / roi->scale[0][VX_DIM_Y]) )
                * roi->scale[0][VX_DIM_X] / roi->memory.strides[0][VX_DIM_X];
}

static vx_status exportPixel(VXBinExport *xport, void *base, vx_uint32 x, vx_uint32 y, vx_uint32 p,
                            vx_imagepatch_addressing_t *addr, vx_df_image format, int calcSize)
{
    /* export a pixel. For some YUV formats the meaning of the values changes for odd pixels */
    vx_status status = VX_FAILURE;
    vx_pixel_value_t *pix = (vx_pixel_value_t *)vxFormatImagePatchAddress2d(base, x, y, addr);
    switch (format)
    {
        case VX_DF_IMAGE_U8:    /* single-plane 1 byte */
        case VX_DF_IMAGE_YUV4:  /* 3-plane 1 byte */
        case VX_DF_IMAGE_IYUV:
            status = exportVxUint8(xport, pix->U8, calcSize);
            break;
        case VX_DF_IMAGE_U16:   /* single-plane U16 */
            status = exportVxUint16(xport, pix->U16, calcSize);
            break;
        case VX_DF_IMAGE_S16:   /* single-plane S16 */
            status = exportVxInt16(xport, pix->S16, calcSize);
            break;
        case VX_DF_IMAGE_U32:   /* single-plane U32 */
            status = exportVxUint32(xport, pix->U32, calcSize);
            break;
        case VX_DF_IMAGE_S32:   /* single-plane S32 */
            status = exportVxInt32(xport, pix->S32, calcSize);
            break;
        case VX_DF_IMAGE_RGB:   /* single-plane 3 bytes */
            status = exportBytes(xport, pix->RGB, 3, calcSize);
            break;
        case VX_DF_IMAGE_RGBX:  /* single-plane 4 bytes */
            status = exportBytes(xport, pix->RGBX, 4, calcSize);
            break;
        case VX_DF_IMAGE_UYVY:  /* single plane, export 2 bytes for UY or VY */
        case VX_DF_IMAGE_YUYV:  /* single plane, export 2 bytes for YU or YV */
            status = exportBytes(xport, pix->YUV, 2, calcSize);
            break;
        case VX_DF_IMAGE_NV12:  /* export 1 byte from 1st plane and 2 from second */
        case VX_DF_IMAGE_NV21:  /* second plane order is different for the two formats */
            if (0 == p)
                status = exportVxUint8(xport, pix->YUV[0], calcSize);     /* Y */
            else
                status = exportBytes(xport, pix->YUV, 2, calcSize);     /* U & V */
            break;
        default:
            DEBUGPRINTF("Unsupported image format");
            status = VX_ERROR_NOT_SUPPORTED;
    }
    return status;
}

static vx_status exportSinglePixel(VXBinExport *xport, void * base, vx_uint32 p,
                                vx_imagepatch_addressing_t *addr, vx_df_image format, int calcSize)
{
    /* export 1st pixel, used for uniform images */
    vx_status status = VX_FAILURE;
    switch (format)
    {
        case VX_DF_IMAGE_U8:    /* single-plane 1 byte */
        case VX_DF_IMAGE_YUV4:  /* 3-plane 1 byte */
        case VX_DF_IMAGE_IYUV:
        case VX_DF_IMAGE_U16:   /* single-plane U16 */
        case VX_DF_IMAGE_S16:   /* single-plane S16 */
        case VX_DF_IMAGE_U32:   /* single-plane U32 */
        case VX_DF_IMAGE_S32:   /* single-plane S32 */
        case VX_DF_IMAGE_RGB:   /* single-plane 3 bytes */
        case VX_DF_IMAGE_RGBX:  /* single-plane 4 bytes */
            status = exportPixel(xport, base, 0, 0, p, addr, format, calcSize);
            break;
        case VX_DF_IMAGE_UYVY:  /* single plane, export 3 bytes for YUV mapping UY0VY1 -> YUV*/
            {
                vx_uint8 *pix = vxFormatImagePatchAddress2d(base, 0, 0, addr);
                status = exportVxUint8(xport, pix[1], calcSize);
                status |= exportVxUint8(xport, pix[0], calcSize);
                status |= exportVxUint8(xport, pix[2], calcSize);
            }
            break;
        case VX_DF_IMAGE_YUYV:  /* single plane, export 3 bytes for YUV mapping Y0UY1V to YUV */
            {
                vx_uint8 *pix = vxFormatImagePatchAddress2d(base, 0, 0, addr);
                status = exportVxUint8(xport, pix[0], calcSize);
                status |= exportVxUint8(xport, pix[1], calcSize);
                status |= exportVxUint8(xport, pix[3], calcSize);
            }
            break;
        case VX_DF_IMAGE_NV12:  /* export 1 byte from 1st plane (Y) and 2 from second */
        case VX_DF_IMAGE_NV21:  /* second plane UV order is different for the two formats */
            {
                vx_uint8 *pix = vxFormatImagePatchAddress2d(base, 0, 0, addr);
                if (0 == p)
                    status = exportVxUint8(xport, pix[0], calcSize);     /* Y */
                else if (VX_DF_IMAGE_NV12 == format)
                {
                    status = exportVxUint8(xport, pix[0], calcSize);     /* U */
                    status |= exportVxUint8(xport, pix[1], calcSize);    /* V */
                }
                else
                {   /* U & V swapped for NV21 */
                    status = exportVxUint8(xport, pix[1], calcSize);     /* U */
                    status |= exportVxUint8(xport, pix[0], calcSize);    /* V */
                }
            }
            break;
        default:
            DEBUGPRINTF("Unsupported pixel format");
            status = VX_ERROR_NOT_SUPPORTED;
    }
    return status;
}

static vx_status exportImage(VXBinExport *xport, const vx_size n, int calcSize)
{
    vx_image image = (vx_image)xport->ref_table[n].ref;
    vx_status status = exportObjectHeader(xport, n, calcSize);                        /* Export common information */
    if (VX_TYPE_IMAGE == image->base.scope->type)
    {
        /* Image is an ROI or image from channel. If parent has same
        number of channels, we assume ROI, otherwise we assume
        image from channel. We don't do any other checks... */
        vx_image parent = image->parent;                        /* Note: parent has already been exported as scope in the object header */
        
        if (parent->planes == image->planes)
        {
            /* This is image from ROI, export the parent and rectangle*/
            vx_uint32 x = 0, y = 0;
            computeROIStartXY(parent, image, &x, &y);                                     /* Find where the ROI starts */
            status = exportVxUint8(xport, (vx_uint8)IMAGE_SUB_TYPE_ROI, calcSize);        /* Export as ROI */
            status |= exportVxUint32(xport, x, calcSize);                                 /* Export the dimensions */
            status |= exportVxUint32(xport, y, calcSize);                                 /* To use to make a vx_rectangle_t */
            status |= exportVxUint32(xport, image->width + x, calcSize); 
            status |= exportVxUint32(xport, image->height + y, calcSize);
        }
        else
        {
            /* This is image from channel, export the parent and channel.
            There are a lot of assumptions here, that the image from channel
            is correctly formed... */
            vx_enum channel = VX_CHANNEL_Y;
            if (image->memory.ptrs[0] != parent->memory.ptrs[0])
            {
                if (image->memory.ptrs[1] == parent->memory.ptrs[1])
                    channel = VX_CHANNEL_U;
                else
                    channel = VX_CHANNEL_Y;
            }
            status = exportVxUint8(xport, (vx_uint8)IMAGE_SUB_TYPE_CHANNEL, calcSize);    /* Export as image from channel */
            status |=  exportVxUint32(xport, (vx_uint32)channel, calcSize);               /* Export channel */
        }
        /* And that's all we do for image from ROI or channel */
    }
    else if (image->parent && VX_TYPE_TENSOR == image->parent->base.type)
    {
        /* object array of images from tensor
            Notice that scope index (of the object array) has already been exported
            in the object header, all other information will be exported with the
            object array.
        */
        status = exportVxUint8(xport, (vx_uint8)IMAGE_SUB_TYPE_TENSOR, calcSize);
    }
    else if (vx_true_e == image->constant)
    {
        /* This is a uniform image */
        status = exportVxUint8(xport, (vx_uint8)IMAGE_SUB_TYPE_UNIFORM, calcSize);
        /* output type */
        status |= exportVxUint32(xport, (vx_uint32)image->width, calcSize);                /* Image width */
        status |= exportVxUint32(xport, (vx_uint32)image->height, calcSize);               /* Image height */
        status |= exportVxUint32(xport, (vx_uint32)image->format, calcSize);               /* Image format */
        /* Now we just output one pixel for each plane as the value to use;
           On import the number of planes is determined by the format, so this relies on
           a properly formed image...
           TODO for version 1.2 we can optimise this since there is a uniform image value attribute.
        */
        {
            vx_rectangle_t rect;
            vx_uint32 p;
            vxGetValidRegionImage(image, &rect);
            rect.end_x =  rect.start_x + 1;
            rect.end_y =  rect.start_y + 1;
            for (p = 0U; p < image->planes; ++p)
            {
                void *base = NULL;
                vx_imagepatch_addressing_t addr;
                vx_map_id id;
                status |= vxMapImagePatch(image, &rect, p, &id, &addr, &base, VX_READ_ONLY, image->memory_type, 0);
                status |= exportSinglePixel(xport, base, p, &addr, image->format, calcSize);
                status |= vxUnmapImagePatch(image, id);
            }
        }
    }
    else if (VX_IX_USE_NO_EXPORT_VALUES == xport->ref_table[n].use && isFromHandle((vx_reference)image))
    {
        /* Image from handle - bugzilla 16312 */
        vx_uint32 p = 0;
        status = exportVxUint8(xport, IMAGE_SUB_TYPE_HANDLE, calcSize);
        status |= exportVxUint32(xport, image->width, calcSize);             /* Image width */
        status |= exportVxUint32(xport, image->height, calcSize);            /* Image height */
        status |= exportVxDfImage(xport, image->format, calcSize);           /* Image format */
        status |= exportVxUint32(xport, image->planes, calcSize);            /* Number of planes, for ease of import */
        status |= exportVxEnum(xport, image->memory_type, calcSize);         /* Memory type */
        for (p = 0; p < image->planes; ++p)
        {
            status |= exportVxInt32(xport, image->memory.strides[p][VX_DIM_X], calcSize);      /* x stride for plane p */
            status |= exportVxInt32(xport, image->memory.strides[p][VX_DIM_Y], calcSize);      /* y stride for plane p */
        }
    }
    else
    {
        /* Regular image */
        status = exportVxUint8(xport, IMAGE_SUB_TYPE_NORMAL, calcSize);
        /* output type */
        status |= exportVxUint32(xport, image->width, calcSize);            /* Image width */
        status |= exportVxUint32(xport, image->height, calcSize);           /* Image height */
        status |= exportVxDfImage(xport, image->format, calcSize);          /* Image format */
        /* Now we have to output the data, but only if use == VX_IX_USE_EXPORT_VALUES
            and not virtual
        */
        if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[n].use && !image->base.is_virtual)
        {
            status |= exportVxUint32(xport, 1U, calcSize);
            vx_rectangle_t rect;
            vx_uint32 p;
            vxGetValidRegionImage(image, &rect);
            /* Output valid region */
            status |= exportVxUint32(xport, rect.start_x, calcSize);
            status |= exportVxUint32(xport, rect.start_y, calcSize);
            status |= exportVxUint32(xport, rect.end_x, calcSize);
            status |= exportVxUint32(xport, rect.end_y, calcSize);
            /* The number of planes is derived from format upon input; here we output all
               the available pixel data.
            */
            for (p = 0U; p < image->planes; ++p)
            {
                void *base = NULL;
                vx_uint32 x, y;
                vx_imagepatch_addressing_t addr;
                vx_map_id id;
                status |= vxMapImagePatch(image, &rect, p, &id, &addr, &base,
                                            VX_READ_ONLY, image->memory_type, 0);
                /* The actual number of data is calculated in the same way upon input;
                   we don't need to export any of step_x, step_y, dim_x, dim_y, x or y.
                */
                for (y = 0U; y < addr.dim_y; y += addr.step_y)
                {
                    for( x = 0U; x < addr.dim_x; x += addr.step_x)
                    {
                        status |= exportPixel(xport, base, x, y, p, &addr, image->format, calcSize);
                    }
                }
                status |= vxUnmapImagePatch(image, id);
            }
        }
        else
        {
             status |= exportVxUint32(xport, 0U, calcSize);
        }
    }
    return status;
}


static vx_status exportLUT(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_lut_t *lut = (vx_lut_t *)xport->ref_table[i].ref;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, (vx_uint32)lut->num_items, calcSize);
    status |= exportVxEnum(xport, lut->item_type, calcSize);
    status |= exportVxUint32(xport, lut->offset, calcSize);
    if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
        vx_false_e == lut->base.is_virtual && lut->memory.ptrs[0])
    {
        vx_uint32 size = lut->num_items * lut->item_size;
        status |= exportVxUint32(xport, size, calcSize);                        		/* Size of data following */
        status |= exportBytes(xport, lut->memory.ptrs[0], size, calcSize);  /* Export the data */
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);                          /* Output a zero to indicate no data */
    }
    return status;
}

static vx_status exportDistribution(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_distribution dist = (vx_distribution)xport->ref_table[i].ref;
    vx_uint32 bins = (vx_uint32)dist->memory.dims[0][VX_DIM_X];
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, bins, calcSize);
    status |= exportVxInt32(xport, dist->offset_x, calcSize);
    status |= exportVxUint32(xport, dist->range_x, calcSize);
    if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
        vx_false_e == dist->base.is_virtual && dist->memory.ptrs[0])
    {
        vx_uint32 size = bins * sizeof(vx_uint32);
        status |= exportVxUint32(xport, size, calcSize);                        /* Size of data following */
        status |= exportBytes(xport, dist->memory.ptrs[0], size, calcSize);    /* Export the data */
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);                          /* Output a zero to indicate no data */
    }
    return status;
}

static vx_status exportThreshold(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_threshold threshold = (vx_threshold)xport->ref_table[i].ref;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxEnum(xport, threshold->thresh_type, calcSize);
    status |= exportVxEnum(xport, threshold->data_type, calcSize);
    status |= exportVxInt32(xport, threshold->value.S32, calcSize);
    status |= exportVxInt32(xport, threshold->lower.S32, calcSize);
    status |= exportVxInt32(xport, threshold->upper.S32, calcSize);
    status |= exportVxInt32(xport, threshold->true_value.S32, calcSize);
    status |= exportVxInt32(xport, threshold->false_value.S32, calcSize);
    return status;
}

static vx_status exportMatrix(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_matrix mat = (vx_matrix)xport->ref_table[i].ref;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, (vx_uint32)mat->columns, calcSize);
    status |= exportVxUint32(xport, (vx_uint32)mat->rows, calcSize);
    status |= exportVxEnum(xport, mat->data_type, calcSize);
    status |= exportVxCoordinates2dT(xport, &mat->origin, calcSize);
    status |= exportVxEnum(xport, mat->pattern, calcSize);
    if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
        vx_false_e == mat->base.is_virtual && mat->memory.ptrs[0])
    {
        vx_uint32 size = mat->columns * mat->rows;
        if (VX_TYPE_FLOAT32 == mat->data_type)       /* can only be int32, float32, or uint8 */
            size *= sizeof(vx_float32);
        else if (VX_TYPE_UINT32 == mat->data_type)
            size *= sizeof(vx_int32);
        status |= exportVxUint32(xport, size, calcSize);                        /* Size of data following */
        status |= exportBytes(xport, mat->memory.ptrs[0], size, calcSize);    /* Export the data */
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);                          /* Output a zero to indicate no data */
    }
    return status;
}

static vx_status exportConvolution(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_convolution conv = (vx_convolution)xport->ref_table[i].ref;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, (vx_uint32)conv->base.columns, calcSize);
    status |= exportVxUint32(xport, (vx_uint32)conv->base.rows, calcSize);
    status |= exportVxUint32(xport, conv->scale, calcSize);
    if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
        vx_false_e == conv->base.base.is_virtual && conv->base.memory.ptrs[0])
    {
        vx_uint32 size = conv->base.columns * conv->base.rows + sizeof(vx_int16);   /* only type supported is int16 */
        status |= exportVxUint32(xport, size, calcSize);                            /* Size of data following */
        status |= exportBytes(xport, conv->base.memory.ptrs[0], size, calcSize);  /* Export the data */
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);                              /* Output a zero to indicate no data */
    }
    return status;
}

static vx_status exportScalar(VXBinExport *xport, const vx_size i, int calcSize)
{
    /* NOTE - now supports new scalar types with user structs (Bugzilla 16338)
     */
    vx_status status = VX_SUCCESS;
    vx_scalar scalar = (vx_scalar)xport->ref_table[i].ref;
    vx_uint32 item_size = ownSizeOfType(scalar->data_type);
    if (item_size == 0ul)
    {
        for (vx_uint32 ix = 0; ix < VX_INT_MAX_USER_STRUCTS; ++ix)
        {
            if (scalar->base.context->user_structs[ix].type == scalar->data_type)
            {
                item_size = scalar->base.context->user_structs[ix].size;
                break;
            }
        }
    }
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxEnum(xport, scalar->data_type, calcSize);
    status |= exportVxUint32(xport, item_size, calcSize);
    if (VX_TYPE_USER_STRUCT_START <= scalar->data_type &&       /* Now output the name if it's a user struct (Bugzilla 16338 comment 2)*/
        VX_TYPE_USER_STRUCT_END >= scalar->data_type)
    {
        int ix = scalar->data_type - VX_TYPE_USER_STRUCT_START;
        if (scalar->base.context->user_structs[ix].name[0])     /* Zero length name not allowed */
            status |= exportBytes(xport, &scalar->base.context->user_structs[ix].name, VX_MAX_STRUCT_NAME, calcSize);
        else
            status = VX_ERROR_INVALID_TYPE;
    }
    if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
        vx_false_e == scalar->base.is_virtual)
    {
        void * data = malloc(item_size);
        status |= exportVxUint32(xport, item_size, calcSize);                       /* Size of data following */
        status |= vxCopyScalar(scalar, data, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);    /* Read the data */
        status |= exportBytes(xport, &scalar->data, item_size, calcSize);           /* Export the data */
        free(data);
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);                          /* Output a zero to indicate no data */
    }
    return status;
}

static vx_status exportArray(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_array arr = (vx_array)xport->ref_table[i].ref;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, (vx_uint32)arr->num_items, calcSize);
    status |= exportVxUint32(xport, arr->capacity, calcSize);
    status |= exportVxEnum(xport, arr->item_type, calcSize);
    status |= exportVxUint32(xport, arr->item_size, calcSize);
    if (VX_TYPE_USER_STRUCT_START <= arr->item_type &&       /* Now output the name if it's a user struct (Bugzilla 16338 comment 2)*/
        VX_TYPE_USER_STRUCT_END >= arr->item_type)
    {
        int ix = arr->item_type - VX_TYPE_USER_STRUCT_START;
        if (arr->base.context->user_structs[ix].name[0])     /* Zero length name not allowed */
            status |= exportBytes(xport, &arr->base.context->user_structs[ix].name, VX_MAX_STRUCT_NAME, calcSize);
        else
            status = VX_ERROR_INVALID_TYPE;
    }
    if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
        vx_false_e == arr->base.is_virtual && arr->memory.ptrs[0])
    {
        vx_uint32 size = arr->num_items * arr->item_size;
        status |= exportVxUint32(xport, size, calcSize);                        /* Size of data following */
        status |= exportBytes(xport, arr->memory.ptrs[0], size, calcSize);      /* Export the data */
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);                          /* Output a zero to indicate no data */
    }
    return status;
}

static vx_status exportRemap(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_remap remap = (vx_remap)xport->ref_table[i].ref;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, remap->src_width, calcSize);
    status |= exportVxUint32(xport, remap->src_height, calcSize);
    status |= exportVxUint32(xport, remap->dst_width, calcSize);
    status |= exportVxUint32(xport, remap->dst_height, calcSize);
    if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
        vx_false_e == remap->base.is_virtual && remap->memory.ptrs[0])
    {
        vx_uint32 size = remap->dst_width * remap->dst_height * sizeof(vx_float32) * 2;
        status |= exportVxUint32(xport, size, calcSize);                        /* Size of data following */
        status |= exportBytes(xport, remap->memory.ptrs[0], size, calcSize);    /* Export the data */
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);                          /* Output a zero to indicate no data */
    }
    return status;
}

static vx_status exportTensor(VXBinExport *xport, const vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_tensor tensor = (vx_tensor)xport->ref_table[i].ref;
    vx_uint32 dim;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, tensor->number_of_dimensions, calcSize);
    status |= exportVxEnum(xport, tensor->data_type, calcSize);
    status |= exportVxUint8(xport, tensor->fixed_point_position, calcSize);
    if (VX_TYPE_TENSOR == tensor->base.scope->type)
    {
        /* export offset of this tensor's memory address from that of its parent */
        status |= exportVxUint32(xport, (vx_uint8 *)tensor->parent->addr - (vx_uint8 *)tensor->addr, calcSize);
    }
    else
    {
        status |= exportVxUint32(xport, 0U, calcSize);        /* No memory pointer offset */
    }
    /* export the size of each dimension */
    for (dim = 0; dim < tensor->number_of_dimensions && VX_SUCCESS == status; ++dim)
    {
        status = exportVxUint32(xport, (vx_uint32)tensor->dimensions[dim], calcSize);
    }
    /* export the stride for each dimension */
    for (dim = 0; dim < tensor->number_of_dimensions && VX_SUCCESS == status; ++dim)
    {
        status = exportVxUint32(xport, (vx_uint32)tensor->stride[dim], calcSize);
    }
    if (VX_SUCCESS == status)
    {
        if (VX_IX_USE_EXPORT_VALUES == xport->ref_table[i].use &&
            vx_false_e == tensor->base.is_virtual &&
            NULL != tensor->addr &&
            VX_TYPE_TENSOR != tensor->base.scope->type)
        {
            vx_uint32 size = tensor->dimensions[tensor->number_of_dimensions - 1] *
                             tensor->stride[tensor->number_of_dimensions - 1];
            status = exportVxUint32(xport, size, calcSize);                        /* Size of data following */
            if (calcSize)
            {
                xport->export_length += size;
            }
            else if (VX_SUCCESS == status)
            {    /* Export the data */
                vx_size starts[VX_MAX_TENSOR_DIMENSIONS] = {0};
                status = vxCopyTensorPatch(tensor, tensor->number_of_dimensions, starts, tensor->dimensions,
                                            tensor->stride, xport->curptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                xport->curptr += size;
            }
        }
        else
        {
            status = exportVxUint32(xport, 0U, calcSize);                        /* Size of data following */
        }
    }
    return status;
}

static vx_status exportKernel(VXBinExport *xport,vx_size i, int calcSize)
{
    /* Bugzilla 16337 - need to export all kernel data*/
    vx_status status = VX_SUCCESS;
    vx_uint32 ix;               /* for enumerating kernel parameters */
    vx_kernel kernel = (vx_kernel)xport->ref_table[i].ref;
    status |= exportObjectHeader(xport, i, calcSize);
    status |= exportVxEnum(xport, kernel->enumeration, calcSize);
    status |= exportBytes(xport, kernel->name, VX_MAX_KERNEL_NAME, calcSize);
    status |= exportVxUint32(xport, kernel->signature.num_parameters, calcSize);
    for (ix = 0; ix < kernel->signature.num_parameters; ++ix)
    {
        status |= exportVxEnum(xport, kernel->signature.directions[ix], calcSize);
        status |= exportVxEnum(xport, kernel->signature.types[ix], calcSize);
        status |= exportVxEnum(xport, kernel->signature.states[ix], calcSize);
    }
    return status;
}

static vx_status exportGraph(VXBinExport *xport,vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 ix;               /* for enumerating graph nodes and delays */
    vx_uint32 delay_count = 0;  /* count of delays */
    vx_graph g = (vx_graph)xport->ref_table[i].ref;
    /* Bugzilla 16207. Allow export of graphs as user kernels; in this case the name
    of the kernel will be the name of the reference to the graph and it must not have
    a zero length */
    if (VX_IX_USE_IMPORT_AS_KERNEL == xport->ref_table[i].use &&
        0 == g->base.name[0])
        status = VX_FAILURE;
    status |= exportObjectHeader(xport, i, calcSize);
    DEBUGPRINTF("Exporting graph with %u parameters\n", g->numParams);
    /* Need to export all delays that are registered with this graph:
        first, count them:
    */
    for (ix = 0; ix < VX_INT_MAX_REF; ++ix)
    {
        if (g->delays[ix])
            ++delay_count;
    }
    status |= exportVxUint32(xport, delay_count, calcSize);
    /* Now actually output each delay reference in turn */
    for (ix = 0; ix < VX_INT_MAX_REF && VX_SUCCESS == status; ++ix)
    {
        if (g->delays[ix])
            status |= exportVxUint32(xport, indexOf(xport, (vx_reference)g->delays[ix]), calcSize);
    }
    /* Now export the number of nodes */
    status |= exportVxUint32(xport, g->numNodes, calcSize);
   /* Now we will export all the nodes of the graph. All nodes are distinct so there
       is no point them being indexed in the ref_table, they are just put here.
    */
    for (ix = 0; ix < g->numNodes && VX_SUCCESS == status; ++ix)
    {
        vx_node node = g->nodes[ix];
        vx_uint32 num_parameters = node->kernel->signature.num_parameters;
        vx_uint32 p;
        /* Bugzilla 16337 - user kernels, we have a kernel export function so we put a link to the kernel here */
        status |= exportVxUint32(xport, indexOf(xport, (vx_reference)node->kernel), calcSize);
        /* The parameter list */
        for (p = 0; p < num_parameters; ++p)
        {
            status |= exportVxUint32(xport, indexOf(xport, node->parameters[p]), calcSize);
        }
        /* Replicated flags */
        status |= exportVxUint8(xport, node->is_replicated ? 1 : 0, calcSize);
        if (node->is_replicated)
        {
            for (p = 0; p < num_parameters; ++p)
            {
                status |= exportVxUint8(xport, node->replicated_flags[p] ? 1 : 0, calcSize);
            }           
        }                        
        /* Affinity */
        status |= exportVxUint32(xport, node->affinity, calcSize);
        /* Attributes */
        status |= exportVxBorderT(xport, &node->attributes.borders, calcSize);
    }
    /* Now output the graph parameters list */
    status |= exportVxUint32(xport, g->numParams, calcSize);
    for (ix = 0; ix < g->numParams && VX_SUCCESS == status; ++ix)
    {
        /* find node in graph's list of nodes and export that index */
        vx_uint32 nix;
        for (nix = 0; nix < g->numNodes; ++nix)
            if (g->nodes[nix] == g->parameters[ix].node)
                break;
        status |= exportVxUint32(xport, nix, calcSize);
        status |= exportVxUint32(xport, g->parameters[ix].index, calcSize);
    }
    /* That is all for the graph */
    return status;
}

static vx_status exportDelay(VXBinExport *xport, vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_delay delay = (vx_delay)xport->ref_table[i].ref;
    vx_uint32 c;
    /* export the delay info */

    status = exportObjectHeader(xport, i, calcSize);            /* Export common information */
    status |= exportVxUint32(xport, delay->count, calcSize);    /* Number of delay slots */
    for (c = 0; c < delay->count && VX_SUCCESS == status; ++c)
    {   
        /* export each slot reference */
        status = exportVxUint32(xport, indexOf(xport, delay->refs[c]), calcSize);
    }
    return status;
}

static vx_status exportObjectArray(VXBinExport *xport, vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_object_array object_array = (vx_object_array)xport->ref_table[i].ref;
    vx_uint32 c;
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, object_array->num_items, calcSize);
    /* Look for the specical case of an object array of images whose parent is a tensor */
    if (VX_TYPE_IMAGE == object_array->items[0]->type && 
        ((vx_image)object_array->items[0])->parent &&
        VX_TYPE_TENSOR == ((vx_image)object_array->items[0])->parent->base.type)
    {
        status |= exportVxEnum(xport, VX_TYPE_TENSOR, calcSize);    /* Indicate type of parent */
        /* we need to output the information required to recreate the object array:
        the number of items (already done), the tensor, the stride, the rectangle
        describing the ROI, and the image format */
        vx_rectangle_t rect;
        vx_uint32 stride = 1;
        vx_image image0 = (vx_image)object_array->items[0];
        vx_tensor tensor = (vx_tensor)image0->parent;
        /* Calculate the start_x and start_y from the memory pointer */
        div_t divmod = div((int64_t)image0->memory.ptrs[0] - (int64_t)tensor->addr, image0->memory.strides[0][1]);
        rect.start_y = divmod.quot;
        rect.start_x = divmod.rem / (image0->memory.strides[0][1] / tensor->dimensions[0]);
        rect.end_y = rect.start_y + image0->height;
        rect.end_x = rect.start_x + image0->width;
        if (object_array->num_items > 1)
        {
            /* Calculate the stride */
            vx_image image1 = (vx_image)object_array->items[1];
            stride = (image1->memory.ptrs[0] - image0->memory.ptrs[0])/image0->memory.strides[0][2];
        }
        status |= exportVxUint32(xport, indexOf(xport, &(tensor->base)), calcSize);
        status |= exportVxUint32(xport, stride, calcSize);
        status |= exportVxUint32(xport, rect.start_x, calcSize);
        status |= exportVxUint32(xport, rect.start_y, calcSize);
        status |= exportVxUint32(xport, rect.end_x, calcSize);
        status |= exportVxUint32(xport, rect.end_y, calcSize);
        status |= exportVxDfImage(xport, image0->format, calcSize);
    }
    else
        status |= exportVxEnum(xport, VX_TYPE_INVALID, calcSize);    /* Indicate type of parent */
    for (c = 0; c < object_array->num_items; ++c)
    {
        status |= exportVxUint32(xport, indexOf(xport, object_array->items[c]), calcSize);
    }
    return status;
}

static vx_status exportPyramid(VXBinExport *xport, vx_size i, int calcSize)
{
    vx_status status = VX_SUCCESS;
    vx_pyramid pyramid = (vx_pyramid)xport->ref_table[i].ref;
    vx_uint32 c;
    /* Export the pyramid info */
    status = exportObjectHeader(xport, i, calcSize);
    status |= exportVxUint32(xport, pyramid->width, calcSize);
    status |= exportVxUint32(xport, pyramid->height, calcSize);
    status |= exportVxDfImage(xport, pyramid->format, calcSize);
    status |= exportVxFloat32(xport, pyramid->scale, calcSize);
    status |= exportVxUint32(xport, pyramid->numLevels, calcSize);
    for (c = 0; c < pyramid->numLevels; ++c)
    {
        /* Export each level */
        status |= exportVxUint32(xport, indexOf(xport, (vx_reference)pyramid->levels[c]), calcSize);
    }
    return status;
}

static vx_status exportObjects(VXBinExport *xport, int calcSize)
{
    vx_size i;      /* for enumerating the references */
    vx_status status;
    xport->curptr = xport->export_buffer;
    status = exportHeader(xport, calcSize);
    for (i = 0; i < xport->actual_numrefs && VX_SUCCESS == status; ++i)
    {
        if (exportToProcess(xport, i, calcSize))
        {
            switch (xport->ref_table[i].ref->type)
            {
                case VX_TYPE_IMAGE:         status = exportImage(xport, i, calcSize);       break;
                case VX_TYPE_LUT:           status = exportLUT(xport, i, calcSize);         break;
                case VX_TYPE_DISTRIBUTION:  status = exportDistribution(xport, i, calcSize);break;
                case VX_TYPE_THRESHOLD:     status = exportThreshold(xport, i, calcSize);   break;
                case VX_TYPE_MATRIX:        status = exportMatrix(xport, i, calcSize);      break;
                case VX_TYPE_CONVOLUTION:   status = exportConvolution(xport, i, calcSize); break;
                case VX_TYPE_SCALAR:        status = exportScalar(xport, i, calcSize);      break;
                case VX_TYPE_ARRAY:         status = exportArray(xport, i, calcSize);       break;
                case VX_TYPE_REMAP:         status = exportRemap(xport, i, calcSize);       break;
                case VX_TYPE_OBJECT_ARRAY:  status = exportObjectArray(xport, i, calcSize); break;
                case VX_TYPE_PYRAMID:       status = exportPyramid(xport, i, calcSize);     break;
                case VX_TYPE_DELAY:         status = exportDelay(xport, i, calcSize);       break;
                case VX_TYPE_GRAPH:         status = exportGraph(xport, i, calcSize);       break;
                case VX_TYPE_TENSOR:        status = exportTensor(xport, i, calcSize);      break;
                case VX_TYPE_KERNEL:        status = exportKernel(xport, i, calcSize);      break;
                default: /* something we don't export at all */
                    break;
            }
        }
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxExportObjectsToMemory (
    vx_context context,
    vx_size numrefs,
    const vx_reference * refs,
    const vx_enum * uses,
    const vx_uint8 ** ptr,
    vx_size * length )
{
    vx_status retval = VX_SUCCESS;      /* To keep track of any errors, and the return value */
    VXBinExport xport = 
        {
            .context = context,
            .refs = refs,
            .uses = uses,
            .ref_table = NULL,
            .numrefs = numrefs,
            .actual_numrefs = numrefs,
            .export_buffer = NULL,                /* Where the data will go */
            .curptr = NULL,
            .export_length = 0          /* Size of the export */
        };
    vx_size i;                          /* For enumerating the refs and uses */
    
    /* ----------- Do all the checks ----------- */
    /* Check initial parameters */
    if (VX_SUCCESS != vxGetStatus((vx_reference)context) ||
        refs == NULL ||     /* no parameters may be NULL */
        uses == NULL ||
        ptr == NULL ||
        length == NULL)
    {
        DEBUGPRINTF("Initial invalid parameters...\n");
        retval = VX_ERROR_INVALID_PARAMETERS;
    }
    else
    {
        /* Check refs and uses array */
        for (i = 0; i < numrefs && VX_SUCCESS == retval; ++i)
        {
            vx_size i2; /*for secondary enumeration of refs and uses array */
            retval = vxGetStatus(refs[i]);  /* All references must be valid */
            if (VX_SUCCESS == retval)
            {
                /* For all objects the context must be the one given */
                if (context != refs[i]->context)
                {
                    DEBUGPRINTF("Wrong context\n");
                    retval = VX_ERROR_INVALID_PARAMETERS;
                }
                else
                {
                    /* If references in 'refs'  have names they must be unique,
                    but note we have to allow for duplicate references */
                    if (refs[i]->name[0] != '\0')
                    {
                        for (i2 = i + 1; i2 < numrefs; ++i2)
                        {
                            if (refs[i] != refs[i2] && 0 == strncmp(refs[i]->name, refs[i2]->name, VX_MAX_REFERENCE_NAME))
                            {
                                DEBUGPRINTF("Duplicate names\n");
                                retval = VX_ERROR_INVALID_PARAMETERS;
                                break;
                            }
                        }
                    }
                }
            }
            if (VX_SUCCESS == retval) 
            {
                switch (refs[i]->type)
                {
			    /* Only graphs and data objects may be exported */
			    case VX_TYPE_GRAPH:
                    if (VX_IX_USE_EXPORT_VALUES != uses[i] )
                    {
                        retval = VX_ERROR_INVALID_PARAMETERS;
                    }
                    else
                    {
                        /* Verify graphs, re-instating their previous state */
                        vx_uint32 ix;   /* for enumerating graph parameters and nodes */
                        vx_graph g = (vx_graph)refs[i];
                        retval = vxVerifyGraph(g);  /* export will fail if verification fails */
                        if (retval)
                        {
                            DEBUGPRINTF("Graph verification failed!\n");
                        }

                        /* All graph parameters are automotically exported with VX_IX_USE_NO_EXPORT_VALUES unless
                        already listed - see 16313. If attached to another graph somewhere *must* be listed */
                        for (ix = 0; VX_SUCCESS == retval && ix < g->numParams; ++ix)
                        {
                            vx_reference r = g->parameters[ix].node->parameters[g->parameters[ix].index];
                            int found = 0;
                            for (i2 = 0; i2 < numrefs && !found; ++i2)
                                found = (refs[i2] == r);
                            if (!found)
                            {
                                /* now check to make sure thay are not on another graph somewhere */
                                for (i2 = i + 1; VX_SUCCESS == retval && i2 < numrefs; ++i2)
                                {
                                    if (VX_TYPE_GRAPH == refs[i2]->type)
                                    {
                                        vx_graph g2 = (vx_graph)refs[i2];
                                        vx_uint32 n = 0;
                                        for (n = 0; VX_SUCCESS == retval && n < g2->numNodes; ++n)
                                        {
                                            vx_uint32 p = 0;
                                            for (p = 0; VX_SUCCESS == retval && p < g2->nodes[n]->kernel->signature.num_parameters; ++p)
                                            {
                                                if (g2->nodes[n]->parameters[p] == r)
                                                {
                                                    DEBUGPRINTF("Unlisted graph parameter found in another graph!\n");
                                                    retval = VX_ERROR_INVALID_PARAMETERS;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        /* "Immutable" node parameters can't be in refs, need to check all nodes in the graph */
                        for (ix = 0; VX_SUCCESS == retval && ix < g->numNodes; ++ix)
                        {
                            vx_node n = g->nodes[ix];
                            vx_kernel k = n->kernel;
                            vx_uint32 p;        /* for enumerating node parameters */
                            for (p = 0; VX_SUCCESS == retval && p < k->signature.num_parameters; ++p)
                                if (isImmutable(k, p))
                                {
                                    vx_reference r = n->parameters[p];
                                    for (i2 = 0; VX_SUCCESS == retval && i2 < numrefs; ++i2)
                                        if (refs[i2] == r)
                                        {
                                            retval = VX_ERROR_INVALID_PARAMETERS;
                                            DEBUGPRINTF("Immutable parameter found\n");
                                        }
                                }
                        }
                    }
                    break;
                case VX_TYPE_IMAGE:
                case VX_TYPE_DELAY:
                case VX_TYPE_LUT:
                case VX_TYPE_DISTRIBUTION:
                case VX_TYPE_PYRAMID:
                case VX_TYPE_THRESHOLD:
                case VX_TYPE_MATRIX:
                case VX_TYPE_CONVOLUTION:
                case VX_TYPE_SCALAR:
                case VX_TYPE_ARRAY:
                case VX_TYPE_REMAP:
                case VX_TYPE_OBJECT_ARRAY:
                case VX_TYPE_TENSOR:
                    {
                        vx_enum use = uses[i];
                        if (refs[i]->is_virtual || /* No exported references may be virtual */
                            /* Objects created from handles must use VX_IX_USE_APPLICATION_CREATE or VX_IS_USE_NO_EXPORT_VALUES (See bugzilla 16312)*/
                            (isFromHandle(refs[i]) && VX_IX_USE_APPLICATION_CREATE != use && VX_IX_USE_NO_EXPORT_VALUES != use) ||
                            /* uses must contain valid enumerants for objects that aren't graphs */
                            (use != VX_IX_USE_APPLICATION_CREATE &&
                             use != VX_IX_USE_EXPORT_VALUES &&
                             use != VX_IX_USE_NO_EXPORT_VALUES
                            )
                           )
                        {
                            retval = VX_ERROR_INVALID_PARAMETERS;
                            DEBUGPRINTF("Virtual or fromhandle found, i = %lu, use = %d, virtual = %s\n",
                                    i, use, refs[i]->is_virtual ? "true" : "false");
                        }
                        break;
                    }
                case VX_TYPE_KERNEL:
                    /* Bugzilla 16207 comment 3. 'graph kernels' may be exported as kernels. The
                    underlying graph is exported.
                    */
                    if (VX_IX_USE_EXPORT_VALUES == uses[i])
                    {
                        if (vx_false_e == ((vx_kernel)refs[i])->user_kernel ||
                            VX_TYPE_GRAPH != refs[i]->scope->type)
                        {
                            /* Kernels being exported this way must be user kernels, and have their scope be a graph. */
                            retval = VX_ERROR_INVALID_PARAMETERS;
                        }
                        else
                        {
                            /* We don't have to process the graph at this stage as it must have already been verified
                            and checked.
                            It will be placed in ref_table for export processing later by putInTable().
                            */
                        }
                    }
                    /* Bugzilla 16337 */
                    else if (VX_IX_USE_APPLICATION_CREATE != uses[i])
                    {
                        DEBUGPRINTF("Wrong use of vx_kernel\n");
                        retval = VX_ERROR_INVALID_PARAMETERS;
                    }
                    break;
                default:
                        /* Disallowed export type */
                    DEBUGPRINTF("Illegal export type\n");
                    retval = VX_ERROR_NOT_SUPPORTED;
                }
            }
        }
    }
    
    /* ----------- Checks done, if OK, try and export ----------- */
    if (VX_SUCCESS == retval)
    {
        /* First, make a table of unique references to export; first allocate an array big enough for any eventuality */
        xport.ref_table = (VXBinExportRefTable *)calloc(sizeof(VXBinExportRefTable), context->num_references);
        xport.actual_numrefs = numrefs;
        /* Now, populate the table and find the actual number of references to export */
        if (xport.ref_table)
        {
            /* First, populate the table with references as is. Thus the first numrefs
               entries in ref_table correspond to refs; there may be duplicates */
            for (i = 0; i < numrefs; ++i)
            {
                xport.ref_table[i].ref = refs[i];
                xport.ref_table[i].use = uses[i];
                xport.ref_table[i].status = OBJECT_UNPROCESSED;
            }
            /* Now mark duplicates */
            for (i = 0; i < numrefs; ++i)
            {
                vx_size i2;     /* for finding duplicates */
                for (i2 = i + 1; i2 < numrefs; ++i2 )
                    if (refs[i] == xport.ref_table[i2].ref)
                        xport.ref_table[i2].status = OBJECT_DUPLICATE;
            }
            /* Next, populate with references from any compound objects; putInTable will expand
               them recursively, but will only process any object with status OBJECT_UNPROCESSED,
               then setting its status to OBJECT_UNSIZED.
            */
            for (i = 0; i < numrefs; ++i)
            {
                xport.actual_numrefs = putInTable(xport.ref_table, refs[i], xport.actual_numrefs, uses[i]);
            }
            /* Calculate the uses for each reference */
            calculateUses(&xport);
            /* Now, calculate the size of the export */
            retval = exportObjects(&xport, 1);
            /* Increment the length of the export to take into account a checksum at the end */
            xport.export_length += sizeof(vx_uint32);
        }
        if (VX_SUCCESS == retval)
        {
            /* Now allocate buffer for the export */
            malloc_export(&xport);
            /* And actually do the export */
            retval = exportObjects(&xport, 0);
            /* Now fix up duplicates */
            for (i = 0; i < numrefs; ++i)
            {
                vx_size i2;     /* for finding duplicates */
                for (i2 = i + 1; i2 < numrefs; ++i2 )
                    if (xport.ref_table[i].ref == xport.ref_table[i2].ref)
                    {
                        /* Make offset of duplicate equal to the offset of the original */
                        memcpy(addressOf(&xport, i2), addressOf(&xport, i), sizeof(vx_uint64));
                    }
            }
            /* Finally, calculate the checksum and place it at the end */
            {
                vx_uint32 checksum = 0;
                xport.curptr = xport.export_buffer;
                for (i = 0; i < xport.export_length - sizeof(checksum); ++i)
                {
                    checksum += *xport.curptr++;
                }
                retval = exportVxUint32(&xport, checksum, 0);
            }
        }
        free(xport.ref_table);
    }
    if (VX_SUCCESS == retval)
    {
        /* On success, set output values */
        *ptr = xport.export_buffer;
        *length = xport.export_length;
    }
    else
    {
        /* On failure, clear output values and release any allocated memory */
        *ptr = NULL;
        *length = 0;
        vxReleaseExportedMemory(context, (const vx_uint8**)&xport.export_buffer);
    }
    return retval;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseExportedMemory(vx_context context, const vx_uint8 ** ptr)
{
    vx_status retval = VX_ERROR_INVALID_PARAMETERS;
    /* First, check we have some exports to release, that the pointer provided is not NULL, and that
        the context is valid. Note that the list of exports is across all contexts */
    if (VX_SUCCESS == vxGetStatus((vx_reference)context) && *ptr && **ptr && num_exports)
    {
        /* next, find the pointer in our list of pointers.*/
        for (int i = 0; i < MAX_CONCURRENT_EXPORTS && VX_SUCCESS != retval; ++i)
        {
            if (*ptr == exports[i])
            {
                /* found it, all good, we can free the memory, zero the pointers and set a successful return value */
                free((void *)*ptr);
                *ptr = NULL;
                exports[i] = NULL;
                --num_exports;
                retval = VX_SUCCESS;
            }
        }
    }
    /* If we haven't found the pointer retval will be unchanged */
    return retval;
}
#endif
