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
 * \brief The PyrLK optical flow kernel.
 * \author Tomer Schwartz
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>

typedef struct _vx_keypoint_t_optpyrlk_internal
{
    vx_float32 x;               /*!< \brief The x coordinate. */
    vx_float32 y;               /*!< \brief The y coordinate. */
    vx_float32 strength;        /*!< \brief The strength of the keypoint. */
    vx_float32 scale;           /*!< \brief Unused field reserved for future use. */
    vx_float32 orientation;     /*!< \brief Unused field reserved for future use. */
    vx_int32   tracking_status; /*!< \brief A zero indicates a lost point. */
    vx_float32 error;           /*!< \brief An tracking method specific error. */

} vx_keypoint_t_optpyrlk_internal;

#define  INT_ROUND(x,n)     (((x) + (1 << ((n)-1))) >> (n))

static vx_status LKTracker(
    const vx_image prevImg, const vx_image prevDerivIx, const vx_image prevDerivIy, const vx_image nextImg,
    const vx_array prevPts, vx_array nextPts,
    vx_scalar winSize_s, vx_scalar criteria_s, vx_uint32 level, vx_scalar epsilon, vx_scalar num_iterations)
{
    vx_status status = VX_FAILURE;

    vx_int32 j;
    vx_size winSize;
    vx_size list_length;
    vx_size list_indx;
    vx_size halfWin;
    const vx_image I       = prevImg;
    const vx_image J       = nextImg;
    const vx_image derivIx = prevDerivIx;
    const vx_image derivIy = prevDerivIy;

    vx_enum termination_Criteria;

    vx_image IWinBuf;
    vx_image derivIWinBuf_x;
    vx_image derivIWinBuf_y;

    vx_df_image derivIx_format = 0;
    vx_df_image J_format       = 0;
    vx_df_image derivIy_format = 0;
    vx_df_image I_format       = 0;
    vx_rectangle_t rect;

    void* derivIx_base = 0;
    void* J_base       = 0;
    void* derivIy_base = 0;
    void* I_base       = 0;

    vx_imagepatch_addressing_t derivIx_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t J_addr       = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t derivIy_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t I_addr       = VX_IMAGEPATCH_ADDR_INIT;


    void* IWinBuf_base        = 0;
    void* derivIWinBuf_x_base = 0;
    void* derivIWinBuf_y_base = 0;

    vx_df_image IWinBuf_format        = 0;
    vx_df_image derivIWinBuf_x_format = 0;
    vx_df_image derivIWinBuf_y_format = 0;

    vx_imagepatch_addressing_t IWinBuf_addr        = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t derivIWinBuf_x_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t derivIWinBuf_y_addr = VX_IMAGEPATCH_ADDR_INIT;

    /* TODO: prevPts_stride and nextPts_stride should be used to access array elements as
           it may differ from sizeof(vx_keypoint_t_optpyrlk_internal)
    */
    vx_size prevPts_stride = 0;
    vx_size nextPts_stride = 0;
    void* prevPtsFirstItem = NULL;
    void* nextPtsFirstItem = NULL;
    vx_map_id prevPtsFirstItem_map_id;
    vx_map_id nextPtsFirstItem_map_id;
    vxQueryArray(prevPts, VX_ARRAY_NUMITEMS, &list_length, sizeof(list_length));

    vxMapArrayRange(prevPts, 0, list_length, &prevPtsFirstItem_map_id, &prevPts_stride, &prevPtsFirstItem, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    vxMapArrayRange(nextPts, 0, list_length, &nextPtsFirstItem_map_id, &nextPts_stride, &nextPtsFirstItem, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

    vx_keypoint_t_optpyrlk_internal* nextPt_item = (vx_keypoint_t_optpyrlk_internal*)nextPtsFirstItem;
    vx_keypoint_t_optpyrlk_internal* prevPt_item = (vx_keypoint_t_optpyrlk_internal*)prevPtsFirstItem;

    vx_int32 num_iterations_i;
    vx_float32 epsilon_f;

    vxCopyScalar(winSize_s, &winSize, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyScalar(num_iterations, &num_iterations_i, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyScalar(epsilon,&epsilon_f,VX_READ_ONLY,VX_MEMORY_TYPE_HOST);
    vxCopyScalar(criteria_s,&termination_Criteria,VX_READ_ONLY,VX_MEMORY_TYPE_HOST);

    halfWin = (vx_size)(winSize*0.5f);

    vx_context context_lk_internal = vxCreateContext();

    IWinBuf        = vxCreateImage(context_lk_internal, (vx_uint32)winSize, (vx_uint32)winSize, VX_DF_IMAGE_S16);
    derivIWinBuf_x = vxCreateImage(context_lk_internal, (vx_uint32)winSize, (vx_uint32)winSize, VX_DF_IMAGE_S16);
    derivIWinBuf_y = vxCreateImage(context_lk_internal, (vx_uint32)winSize, (vx_uint32)winSize, VX_DF_IMAGE_S16);

    ownInitImage((vx_image_t*)IWinBuf, (vx_uint32)winSize, (vx_uint32)winSize, VX_DF_IMAGE_S16);
    ownAllocateImage((vx_image_t*)IWinBuf);
    ownInitImage((vx_image_t*)derivIWinBuf_x, (vx_uint32)winSize, (vx_uint32)winSize, VX_DF_IMAGE_S16);
    ownAllocateImage((vx_image_t*)derivIWinBuf_x);
    ownInitImage((vx_image_t*)derivIWinBuf_y, (vx_uint32)winSize, (vx_uint32)winSize, VX_DF_IMAGE_S16);
    ownAllocateImage((vx_image_t*)derivIWinBuf_y);

    vxQueryImage(derivIx, VX_IMAGE_FORMAT, &derivIx_format, sizeof(derivIx_format));
    vxQueryImage(J, VX_IMAGE_FORMAT, &J_format, sizeof(J_format));
    vxQueryImage(derivIy, VX_IMAGE_FORMAT, &derivIy_format, sizeof(derivIy_format));
    vxQueryImage(I, VX_IMAGE_FORMAT, &I_format, sizeof(I_format));

    //vxGetValidRegionImage(derivIx,&rect);
    rect.start_x = 0;
    rect.start_y = 0;
    vxQueryImage(derivIx, VX_IMAGE_WIDTH,  &rect.end_x, sizeof(rect.end_x));
    vxQueryImage(derivIx, VX_IMAGE_HEIGHT, &rect.end_y, sizeof(rect.end_y));

    status = VX_SUCCESS;

    vx_map_id map_id_derivIx = 0;
    vx_map_id map_id_J = 0;
    vx_map_id map_id_derivIy = 0;
    vx_map_id map_id_I = 0;
    status |= vxMapImagePatch(derivIx, &rect, 0, &map_id_derivIx, &derivIx_addr, (void**)&derivIx_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(J, &rect, 0, &map_id_J, &J_addr, (void**)&J_base,VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(derivIy, &rect, 0, &map_id_derivIy, &derivIy_addr, (void**)&derivIy_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(I, &rect, 0, &map_id_I, &I_addr, (void**)&I_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

    vxQueryImage(IWinBuf,        VX_IMAGE_FORMAT, &IWinBuf_format,        sizeof(IWinBuf_format));
    vxQueryImage(derivIWinBuf_x, VX_IMAGE_FORMAT, &derivIWinBuf_x_format, sizeof(derivIWinBuf_x_format));
    vxQueryImage(derivIWinBuf_y, VX_IMAGE_FORMAT, &derivIWinBuf_y_format, sizeof(derivIWinBuf_y_format));

    //vxGetValidRegionImage(IWinBuf,&rect);
    rect.start_x = 0;
    rect.start_y = 0;
    vxQueryImage(IWinBuf, VX_IMAGE_WIDTH,  &rect.end_x, sizeof(rect.end_x));
    vxQueryImage(IWinBuf, VX_IMAGE_HEIGHT, &rect.end_y, sizeof(rect.end_y));

    vx_map_id map_id_IWinBuf = 0;
    vx_map_id map_id_x = 0;
    vx_map_id map_id_y = 0;
    status |= vxMapImagePatch(IWinBuf,        &rect, 0, &map_id_IWinBuf, &IWinBuf_addr,        (void**)&IWinBuf_base,        VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(derivIWinBuf_x, &rect, 0, &map_id_x, &derivIWinBuf_x_addr, (void**)&derivIWinBuf_x_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(derivIWinBuf_y, &rect, 0, &map_id_y, &derivIWinBuf_y_addr, (void**)&derivIWinBuf_y_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);

    for(list_indx = 0; list_indx < list_length; list_indx++)
    {
        // Does not compute a point that shouldn't be tracked
        if (prevPt_item[list_indx].tracking_status == 0)
            continue;

        vx_keypoint_t_optpyrlk_internal nextPt;
        vx_keypoint_t_optpyrlk_internal prevPt;
        vx_keypoint_t_optpyrlk_internal iprevPt;
        vx_keypoint_t_optpyrlk_internal inextPt;

        prevPt.x = prevPt_item[list_indx].x - halfWin;
        prevPt.y = prevPt_item[list_indx].y - halfWin;

        nextPt.x = nextPt_item[list_indx].x - halfWin;
        nextPt.y = nextPt_item[list_indx].y - halfWin;

        iprevPt.x = floorf(prevPt.x);
        iprevPt.y = floorf(prevPt.y);

        if( iprevPt.x < 0 || iprevPt.x >= derivIx_addr.dim_x - winSize-1 ||
            iprevPt.y < 0 || iprevPt.y >= derivIx_addr.dim_y - winSize-1 )
        {
            if( level == 0 )
            {
                nextPt_item[list_indx].tracking_status = 0;
                nextPt_item[list_indx].error           = 0;
            }
            continue;
        }

        vx_float32 a = prevPt.x - iprevPt.x;
        vx_float32 b = prevPt.y - iprevPt.y;
        const vx_int32 W_BITS  = 14;
        const vx_int32 W_BITS1 = 14;
        const vx_float32 FLT_SCALE = 1.f/(1 << 20);

        vx_int32 iw00 = (vx_int32)(((1.f - a)*(1.f - b)*(1 << W_BITS))+0.5f);
        vx_int32 iw01 = (vx_int32)((a*(1.f - b)*(1 << W_BITS))+0.5f);
        vx_int32 iw10 = (vx_int32)(((1.f - a)*b*(1 << W_BITS))+0.5f);
        vx_int32 iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;

        vx_int32 dstep_x = (vx_int32)(derivIx_addr.stride_y)/2;
        vx_int32 dstep_y = (vx_int32)(derivIy_addr.stride_y)/2;
        vx_int32 stepJ   = (vx_int32)(J_addr.stride_y);
        vx_int32 stepI   = (vx_int32)(I_addr.stride_y);

        vx_float64 A11 = 0;
        vx_float64 A12 = 0;
        vx_float64 A22 = 0;

        // extract the patch from the first image, compute covariation matrix of derivatives
        vx_int32 x;
        vx_int32 y;
        for( y = 0; y < (vx_int32)winSize; y++ )
        {
            vx_uint8* src = (vx_uint8*)vxFormatImagePatchAddress2d(I_base, (vx_uint32)iprevPt.x, (vx_uint32)(y + iprevPt.y), &I_addr);
            vx_int16* dsrc_x = (vx_int16*)vxFormatImagePatchAddress2d(derivIx_base, (vx_uint32)iprevPt.x, (vx_uint32)(y + iprevPt.y), &derivIx_addr);
            vx_int16* dsrc_y = (vx_int16*)vxFormatImagePatchAddress2d(derivIy_base, (vx_uint32)iprevPt.x, (vx_uint32)(y + iprevPt.y), &derivIy_addr);

            vx_int16* Iptr    = (vx_int16*)vxFormatImagePatchAddress2d(IWinBuf_base,       0, y, &IWinBuf_addr);
            vx_int16* dIptr_x = (vx_int16*)vxFormatImagePatchAddress2d(derivIWinBuf_x_base,0, y, &derivIWinBuf_x_addr);
            vx_int16* dIptr_y = (vx_int16*)vxFormatImagePatchAddress2d(derivIWinBuf_y_base,0, y, &derivIWinBuf_y_addr);

            x = 0;

            for( ; x < (vx_int32)winSize; x++, dsrc_x ++, dsrc_y ++)
            {
                vx_int32 ival  = INT_ROUND(src[x]*iw00 + src[x+1]*iw01 +
                                      src[x+stepI]*iw10 + src[x+stepI+1]*iw11, W_BITS1-5);
                vx_int32 ixval = INT_ROUND(dsrc_x[0]*iw00 + dsrc_x[1]*iw01 +
                                      dsrc_x[dstep_x]*iw10 + dsrc_x[dstep_x+1]*iw11, W_BITS1);
                vx_int32 iyval = INT_ROUND(dsrc_y[0]*iw00 + dsrc_y[1]*iw01 + dsrc_y[dstep_y]*iw10 +
                                      dsrc_y[dstep_y+1]*iw11, W_BITS1);

                Iptr[x]    = (vx_int16)ival;
                dIptr_x[x] = (vx_int16)ixval;
                dIptr_y[x] = (vx_int16)iyval;

                A11 += (vx_float32)(ixval*ixval);
                A12 += (vx_float32)(ixval*iyval);
                A22 += (vx_float32)(iyval*iyval);
            }
        }

        A11 *= FLT_SCALE;
        A12 *= FLT_SCALE;
        A22 *= FLT_SCALE;

        vx_float64 D = A11*A22 - A12*A12;
        vx_float32 minEig = (vx_float32)(A22 + A11 - sqrt((A11-A22)*(A11-A22) +
                        4.f*A12*A12))/(2*winSize*winSize);

        if( minEig < 1.0e-04F || D < 1.0e-07F )
        {
            if( level == 0 )
            {
                nextPt_item[list_indx].tracking_status = 0;
                nextPt_item[list_indx].error           = 0;
            }
            continue;
        }

        D = 1.f/D;

        vx_float32 prevDelta_x = 0.0f;
        vx_float32 prevDelta_y = 0.0f;

        j = 0;
        while(j < num_iterations_i || termination_Criteria == VX_TERM_CRITERIA_EPSILON)
        {
            inextPt.x = floorf(nextPt.x);
            inextPt.y = floorf(nextPt.y);

            if( inextPt.x < 0 || inextPt.x >= J_addr.dim_x - winSize-1 ||
                inextPt.y < 0 || inextPt.y >= J_addr.dim_y - winSize-1 )
            {
                if( level == 0  )
                {
                    nextPt_item[list_indx].tracking_status = 0;
                    nextPt_item[list_indx].error           = 0;
                }
                break;
            }

            a = nextPt.x - inextPt.x;
            b = nextPt.y - inextPt.y;
            iw00 = (vx_int32)(((1.f - a)*(1.f - b)*(1 << W_BITS))+0.5);
            iw01 = (vx_int32)((a*(1.f - b)*(1 << W_BITS))+0.5);
            iw10 = (vx_int32)(((1.f - a)*b*(1 << W_BITS))+0.5);
            iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;

            vx_float64 b1 = 0;
            vx_float64 b2 = 0;

            for( y = 0; y < (vx_int32)winSize; y++ )
            {
                vx_uint8* Jptr = (vx_uint8*)vxFormatImagePatchAddress2d(J_base, (vx_uint32)inextPt.x, (vx_uint32)(y + inextPt.y), &J_addr);
                vx_int16* Iptr    = (vx_int16*)vxFormatImagePatchAddress2d(IWinBuf_base,        0, y, &IWinBuf_addr);
                vx_int16* dIptr_x = (vx_int16*)vxFormatImagePatchAddress2d(derivIWinBuf_x_base, 0, y, &derivIWinBuf_x_addr);
                vx_int16* dIptr_y = (vx_int16*)vxFormatImagePatchAddress2d(derivIWinBuf_y_base, 0, y, &derivIWinBuf_y_addr);

                x = 0;

                for( ; x < (vx_int32)winSize; x++)
                {
                    vx_int32 diff = INT_ROUND(Jptr[x]*iw00 + Jptr[x+1]*iw01 +
                                         Jptr[x+stepJ]*iw10 + Jptr[x+stepJ+1]*iw11,
                                         W_BITS1-5) - Iptr[x];
                    b1 += (vx_float32)(diff*dIptr_x[x]);
                    b2 += (vx_float32)(diff*dIptr_y[x]);
                }
            }

            b1 *= FLT_SCALE;
            b2 *= FLT_SCALE;

            vx_float32 delta_x = (vx_float32)((A12*b2 - A22*b1) * D);
            vx_float32 delta_y = (vx_float32)((A12*b1 - A11*b2) * D);

            nextPt.x += delta_x;
            nextPt.y += delta_y;
            nextPt_item[list_indx].x = nextPt.x + halfWin;
            nextPt_item[list_indx].y = nextPt.y + halfWin;

            if( (delta_x*delta_x + delta_y*delta_y) <= epsilon_f &&
                (termination_Criteria == VX_TERM_CRITERIA_EPSILON || termination_Criteria == VX_TERM_CRITERIA_BOTH))
                break;

            if( j > 0 && fabs(delta_x + prevDelta_x) < 0.01 &&
               fabs(delta_y + prevDelta_y) < 0.01 )
            {
                nextPt_item[list_indx].x -= delta_x*0.5f;
                nextPt_item[list_indx].y -= delta_y*0.5f;
                break;
            }
            prevDelta_x = delta_x;
            prevDelta_y = delta_y;
            j++;
        }
    }

    status |= vxUnmapArrayRange(prevPts, prevPtsFirstItem_map_id);
    status |= vxUnmapArrayRange(nextPts, nextPtsFirstItem_map_id);
    /* no needs to commit changes in temporary images */
    status |= vxUnmapImagePatch(IWinBuf, map_id_IWinBuf);
    status |= vxUnmapImagePatch(derivIWinBuf_x, map_id_x);
    status |= vxUnmapImagePatch(derivIWinBuf_y, map_id_y);
    /* no needs to commit for read-only images */
    status |= vxUnmapImagePatch(derivIx, map_id_derivIx);
    status |= vxUnmapImagePatch(J, map_id_J);
    status |= vxUnmapImagePatch(derivIy, map_id_derivIy);
    status |= vxUnmapImagePatch(I, map_id_I);

    ownFreeImage((vx_image_t*)IWinBuf);
    ownFreeImage((vx_image_t*)derivIWinBuf_x);
    ownFreeImage((vx_image_t*)derivIWinBuf_y);

    vxReleaseImage(&IWinBuf);
    vxReleaseImage(&derivIWinBuf_x);
    vxReleaseImage(&derivIWinBuf_y);

    vxReleaseContext(&context_lk_internal);

    return VX_SUCCESS;
}


static vx_param_description_t optpyrlk_kernel_params[] =
{
    {VX_INPUT, VX_TYPE_PYRAMID, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_PYRAMID, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
};

static vx_status VX_CALLBACK vxOpticalFlowPyrLKKernel(vx_node node, const vx_reference* parameters, vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == dimof(optpyrlk_kernel_params))
    {
        vx_size maxLevel;
        vx_size list_length;
        vx_size list_indx;
        vx_int32 level;
        vx_pyramid old_pyramid          = (vx_pyramid)parameters[0];
        vx_pyramid new_pyramid          = (vx_pyramid)parameters[1];
        vx_array   prevPts              = (vx_array)parameters[2];
        vx_array   estimatedPts         = (vx_array)parameters[3];
        vx_array   nextPts              = (vx_array)parameters[4];
        vx_scalar  termination          = (vx_scalar)parameters[5];
        vx_scalar  epsilon              = (vx_scalar)parameters[6];
        vx_scalar  num_iterations       = (vx_scalar)parameters[7];
        vx_scalar  use_initial_estimate = (vx_scalar)parameters[8];
        vx_scalar  window_dimension     = (vx_scalar)parameters[9];
        vx_size prevPts_stride = 0;
        vx_size estimatedPts_stride = 0;
        vx_size nextPts_stride = 0;
        void* prevPtsFirstItem;
        void* initialPtsFirstItem;
        void* nextPtsFirstItem;

        vx_bool use_initial_estimate_b;
        vx_float32 pyramid_scale;
        vx_keypoint_t* initialPt = NULL;
        vx_keypoint_t_optpyrlk_internal* prevPt;
        vx_keypoint_t_optpyrlk_internal* nextPt = NULL;

        vxCopyScalar(use_initial_estimate,&use_initial_estimate_b,VX_READ_ONLY,VX_MEMORY_TYPE_HOST);

        vxQueryPyramid(old_pyramid, VX_PYRAMID_LEVELS, &maxLevel, sizeof(maxLevel));
        vxQueryPyramid(old_pyramid, VX_PYRAMID_SCALE, &pyramid_scale, sizeof(pyramid_scale));

        // the point in the list are in integer coordinates of x,y
        // The algorithm demand a float so we convert the point first.
        vxQueryArray(prevPts, VX_ARRAY_NUMITEMS, &list_length, sizeof(list_length));

        for( level = (vx_int32)maxLevel; level > 0; level-- )
        {
            vx_image old_image = vxGetPyramidLevel(old_pyramid, level-1);
            vx_image new_image = vxGetPyramidLevel(new_pyramid, level-1);
            vx_rectangle_t rect;
            vx_map_id nextPtsFirstItem_map_id;
            vx_map_id initialPtsFirstItem_map_id;
            vx_map_id prevPtsFirstItem_map_id;

            prevPtsFirstItem = NULL;
            vxMapArrayRange(prevPts, 0, list_length, &prevPtsFirstItem_map_id, &prevPts_stride, &prevPtsFirstItem,
                               VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
            prevPt = (vx_keypoint_t_optpyrlk_internal*)prevPtsFirstItem;

            if (level == maxLevel)
            {
                if (use_initial_estimate_b)
                {
                    vx_size list_length2 = 0;
                    vxQueryArray(estimatedPts, VX_ARRAY_NUMITEMS, &list_length2, sizeof(list_length2));
                    if (list_length2 != list_length)
                        return VX_ERROR_INVALID_PARAMETERS;

                    initialPtsFirstItem = NULL;
                    vxMapArrayRange(estimatedPts, 0, list_length, &initialPtsFirstItem_map_id, &estimatedPts_stride, &initialPtsFirstItem,
                                       VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                    initialPt = (vx_keypoint_t*)initialPtsFirstItem;
                }
                else
                {
                    initialPt = (vx_keypoint_t*)prevPt;
                }

                vxTruncateArray(nextPts, 0);
            }
            else
            {
                nextPtsFirstItem = NULL;
                vxMapArrayRange(nextPts, 0, list_length, &nextPtsFirstItem_map_id,  &nextPts_stride, &nextPtsFirstItem,
                                VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
                nextPt = (vx_keypoint_t_optpyrlk_internal*)nextPtsFirstItem;
            }

            for(list_indx = 0; list_indx < list_length; list_indx++)
            {
                if(level == maxLevel)
                {
                    vx_keypoint_t_optpyrlk_internal keypoint;

                    /* Adjust the prevPt coordinates to the level */
                    (prevPt)->x = (((vx_keypoint_t*)prevPt))->x * (vx_float32)((pow(pyramid_scale, level-1)));
                    (prevPt)->y = (((vx_keypoint_t*)prevPt))->y * (vx_float32)((pow(pyramid_scale, level-1)));

                    if (use_initial_estimate_b)
                    {
                        /* Estimate point coordinates not already adjusted */
                        keypoint.x = (initialPt)->x * (vx_float32)((pow(pyramid_scale, level-1)));
                        keypoint.y = (initialPt)->y * (vx_float32)((pow(pyramid_scale, level-1)));
                    }
                    else
                    {
                        /* prevPt coordinates already adjusted */
                        keypoint.x = (prevPt)->x;
                        keypoint.y = (prevPt)->y;
                    }

                    keypoint.strength        = (initialPt)->strength;
                    keypoint.tracking_status = (initialPt)->tracking_status;
                    keypoint.error           = (initialPt)->error;
                    keypoint.scale           = 0.0f; // init unused field
                    keypoint.orientation     = 0.0f; // init unused field

                    status |= vxAddArrayItems(nextPts, 1, &keypoint, sizeof(keypoint));
                }
                else
                {
                    (prevPt)->x = (prevPt)->x * (1.0f/pyramid_scale);
                    (prevPt)->y = (prevPt)->y * (1.0f/pyramid_scale);
                    (nextPt)->x = (nextPt)->x * (1.0f/pyramid_scale);
                    (nextPt)->y = (nextPt)->y * (1.0f/pyramid_scale);

                    nextPt++;
                }

                prevPt++;
                initialPt++;
            }
            vxUnmapArrayRange(prevPts, prevPtsFirstItem_map_id);

            if (level != maxLevel)
            {
                vxUnmapArrayRange(nextPts, nextPtsFirstItem_map_id);
            }
            else if (use_initial_estimate_b)
            {
                vxUnmapArrayRange(estimatedPts, initialPtsFirstItem_map_id);
            }

            vxGetValidRegionImage(old_image,&rect);
            {
                vx_context context_scharr = vxCreateContext();
                if(vxGetStatus((vx_reference)context_scharr) == VX_SUCCESS)
                {
                    vx_uint32 n;
                    vx_uint32 width;
                    vx_uint32 height;

                    width  = rect.end_x - rect.start_x;
                    height = rect.end_y - rect.start_y;

                    vx_status extras_status = vxLoadKernels(context_scharr, "openvx-extras");

                    vx_graph graph_scharr = vxCreateGraph(context_scharr);

                    if (vxGetStatus((vx_reference)graph_scharr) == VX_SUCCESS && extras_status == VX_SUCCESS)
                    {
                        vx_image shar_images[] =
                        {
                            old_image,                                                     // index 0: Input
                            vxCreateImage(context_scharr, width, height, VX_DF_IMAGE_S16), // index 1: Output dx
                            vxCreateImage(context_scharr, width, height, VX_DF_IMAGE_S16), // index 2: Output dy
                        };

                        vx_node scharr_nodes[] =
                        {
                            vxScharr3x3Node(graph_scharr, shar_images[0], shar_images[1], shar_images[2]),
                        };

                        status = vxVerifyGraph(graph_scharr);
                        if (status == VX_SUCCESS)
                        {

                            status = vxProcessGraph(graph_scharr);

                            status |= LKTracker(
                                old_image, shar_images[1], shar_images[2],
                                new_image,
                                prevPts, nextPts,
                                window_dimension, termination, level-1, epsilon, num_iterations);
                        }

                        for (n = 1; n < dimof(shar_images); n++)
                        {
                            vxReleaseImage(&shar_images[n]);
                        }

                        for (n = 0; n < dimof(scharr_nodes); n++)
                        {
                            vxReleaseNode(&scharr_nodes[n]);
                        }

                        vxReleaseGraph(&graph_scharr);
                    }

                    vxUnloadKernels(context_scharr, "openvx-extras");
                    vxReleaseContext(&context_scharr);
                }
            }

            vxReleaseImage(&new_image);
            vxReleaseImage(&old_image);
        }
        vx_map_id prevPtsFirstItem_map_id;
        vx_map_id nextPtsFirstItem_map_id;
        prevPtsFirstItem = NULL;
        nextPtsFirstItem = NULL;

        vxMapArrayRange(prevPts, 0, list_length, &prevPtsFirstItem_map_id, &prevPts_stride, &prevPtsFirstItem,
                        VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        vxMapArrayRange(nextPts, 0, list_length, &nextPtsFirstItem_map_id, &nextPts_stride, &nextPtsFirstItem,
                        VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        nextPt    = (vx_keypoint_t_optpyrlk_internal*)nextPtsFirstItem;
        prevPt    = (vx_keypoint_t_optpyrlk_internal*)prevPtsFirstItem;

        for(list_indx = 0; list_indx < list_length; list_indx++)
        {
            (((vx_keypoint_t*)nextPt))->x = (vx_int32)((nextPt)->x + 0.5f);
            (((vx_keypoint_t*)nextPt))->y = (vx_int32)((nextPt)->y + 0.5f);
            (((vx_keypoint_t*)prevPt))->x = (vx_int32)((prevPt)->x + 0.5f);
            (((vx_keypoint_t*)prevPt))->y = (vx_int32)((prevPt)->y + 0.5f);
            nextPt++;
            prevPt++;
        }

        vxUnmapArrayRange(prevPts, prevPtsFirstItem_map_id);
        vxUnmapArrayRange(nextPts, nextPtsFirstItem_map_id);
       return status;
    }

    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxOpticalFlowPyrLKInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0 || index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_pyramid input = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if (input)
            {
                vx_size level = 0;
                vxQueryPyramid(input, VX_PYRAMID_LEVELS, &level, sizeof(level));
                if (level !=0)
                {
                    status = VX_SUCCESS;
                }
                vxReleasePyramid(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2 || index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_array arr = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &arr, sizeof(arr));
            if (arr)
            {
                vx_enum item_type = 0;
                vxQueryArray(arr, VX_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
                if (item_type == VX_TYPE_KEYPOINT)
                {
                    status = VX_SUCCESS;
                }
                vxReleaseArray(&arr);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 5)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
         if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
         {
             vx_scalar sens = 0;
             status = vxQueryParameter(param, VX_PARAMETER_REF, &sens, sizeof(sens));
             if ((status == VX_SUCCESS) && (sens))
             {
                 vx_enum type = 0;
                 vxQueryScalar(sens, VX_SCALAR_TYPE, &type, sizeof(type));
                 if (type == VX_TYPE_ENUM)
                 {
                     status = VX_SUCCESS;
                 }
                 else
                 {
                     status = VX_ERROR_INVALID_TYPE;
                 }
                 vxReleaseScalar(&sens);
             }
             vxReleaseParameter(&param);
         }
    }
    else if (index == 6)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
         if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
         {
             vx_scalar sens = 0;
             status = vxQueryParameter(param, VX_PARAMETER_REF, &sens, sizeof(sens));
             if ((status == VX_SUCCESS) && (sens))
             {
                 vx_enum type = 0;
                 vxQueryScalar(sens, VX_SCALAR_TYPE, &type, sizeof(type));
                 if (type == VX_TYPE_FLOAT32)
                 {
                     status = VX_SUCCESS;
                 }
                 else
                 {
                     status = VX_ERROR_INVALID_TYPE;
                 }
                 vxReleaseScalar(&sens);
             }
             vxReleaseParameter(&param);
         }
    }
    else if (index == 7)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
         if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
         {
             vx_scalar sens = 0;
             status = vxQueryParameter(param, VX_PARAMETER_REF, &sens, sizeof(sens));
             if ((status == VX_SUCCESS) && (sens))
             {
                 vx_enum type = 0;
                 vxQueryScalar(sens, VX_SCALAR_TYPE, &type, sizeof(type));
                 if (type == VX_TYPE_UINT32)
                 {
                     status = VX_SUCCESS;
                 }
                 else
                 {
                     status = VX_ERROR_INVALID_TYPE;
                 }
                 vxReleaseScalar(&sens);
             }
             vxReleaseParameter(&param);
         }
    }
    else if (index == 8)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar sens = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &sens, sizeof(sens));
            if ((status == VX_SUCCESS) && (sens))
            {
                vx_enum type = 0;
                vxQueryScalar(sens, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_BOOL)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&sens);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 9)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
         if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
         {
             vx_scalar sens = 0;
             status = vxQueryParameter(param, VX_PARAMETER_REF, &sens, sizeof(sens));
             if ((status == VX_SUCCESS) && (sens))
             {
                 vx_enum type = 0;
                 vxQueryScalar(sens, VX_SCALAR_TYPE, &type, sizeof(type));
                 if (type == VX_TYPE_SIZE)
                 {
                     status = VX_SUCCESS;
                 }
                 else
                 {
                     status = VX_ERROR_INVALID_TYPE;
                 }
                 vxReleaseScalar(&sens);
             }
             vxReleaseParameter(&param);
         }
    }
    return status;
}

static vx_status VX_CALLBACK vxOpticalFlowPyrLKOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 4)
    {
        vx_array arr = 0;
        vx_size capacity = 0;
        vx_parameter param = vxGetParameterByIndex(node, 2);
        vxQueryParameter(param, VX_PARAMETER_REF, &arr, sizeof(arr));
        vxQueryArray(arr, VX_ARRAY_CAPACITY, &capacity, sizeof(capacity));

        ptr->type = VX_TYPE_ARRAY;
        ptr->dim.array.item_type = VX_TYPE_KEYPOINT;
        ptr->dim.array.capacity = capacity;

        status = VX_SUCCESS;

        vxReleaseArray(&arr);
        vxReleaseParameter(&param);
    }
    return status;
}

#ifdef __cplusplus
extern "C"
#endif
vx_kernel_description_t optpyrlk_kernel =
{
    VX_KERNEL_OPTICAL_FLOW_PYR_LK,
    "org.khronos.openvx.optical_flow_pyr_lk",
    vxOpticalFlowPyrLKKernel,
    optpyrlk_kernel_params, dimof(optpyrlk_kernel_params),
    NULL,
    vxOpticalFlowPyrLKInputValidator,
    vxOpticalFlowPyrLKOutputValidator,
    NULL,
    NULL,
};
