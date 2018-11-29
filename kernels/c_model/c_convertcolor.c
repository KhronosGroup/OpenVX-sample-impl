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

#include <c_model.h>
#include <vx_debug.h>

// helpers -------------------------------------------------------------------

static vx_uint8 usat8(vx_int32 a)
{
    if (a > 255)
        a = 255;
    if (a < 0)
        a = 0;
    return (vx_uint8)a;
}

static void yuv2rgb_bt601(vx_uint8 y, vx_uint8 cb, vx_uint8 cr,
                          vx_uint8 *r, vx_uint8 *g, vx_uint8 *b)
{
    /*
    R'= Y' + 0.000*U' + 1.403*V'
    G'= Y' - 0.344*U' - 0.714*V'
    B'= Y' + 1.773*U' + 0.000*V'
    */
    vx_float64 f_y = (vx_float64)y;
    vx_float64 f_u = (vx_float64)cb - 128;
    vx_float64 f_v = (vx_float64)cr - 128;
    vx_float64 f_r = f_y + 0.000f*f_u + 1.403f*f_v;
    vx_float64 f_g = f_y - 0.344f*f_u - 0.714f*f_v;
    vx_float64 f_b = f_y + 1.773f*f_u + 0.000f*f_v;
    vx_int32 i_r = (vx_int32)f_r;
    vx_int32 i_g = (vx_int32)f_g;
    vx_int32 i_b = (vx_int32)f_b;
    *r = usat8(i_r);
    *g = usat8(i_g);
    *b = usat8(i_b);
}

#if 0 /* we don't make 601 yet */
static void rgb2yuv_bt601(vx_uint8 r, vx_uint8 g, vx_uint8 b,
                          vx_uint8 *y, vx_uint8 *cb, vx_uint8 *cr)
{
    /*
    Y'= 0.299*R' + 0.587*G' + 0.114*B'
    Cb=-0.169*R' - 0.331*G' + 0.500*B'
    Cr= 0.500*R' - 0.419*G' - 0.081*B'
    */
    vx_float64 f_r = (vx_float64)r;
    vx_float64 f_g = (vx_float64)g;
    vx_float64 f_b = (vx_float64)b;
    vx_float64 f_y = 0 + 0.299f*f_r + 0.587f*f_g + 0.114f*f_b;
    vx_float64 f_u = 0 - 0.169f*f_r - 0.331f*f_g + 0.500f*f_b;
    vx_float64 f_v = 0 + 0.500f*f_r - 0.419f*f_g - 0.081f*f_b;
    vx_int32 i_y = (vx_int32)f_y;
    vx_int32 i_u = (vx_int32)f_u + 128;
    vx_int32 i_v = (vx_int32)f_v + 128;
    *y  = usat8(i_y);
    *cb = usat8(i_u);
    *cr = usat8(i_v);
}
#endif

static void yuv2rgb_bt709(vx_uint8 y, vx_uint8 cb, vx_uint8 cr,
                          vx_uint8 *r, vx_uint8 *g, vx_uint8 *b)
{
    /*
    R'= Y' + 0.0000*U + 1.5748*V
    G'= Y' - 0.1873*U - 0.4681*V
    B'= Y' + 1.8556*U + 0.0000*V
    */
    vx_float64 f_y = (vx_float64)y;
    vx_float64 f_u = (vx_float64)cb - 128;
    vx_float64 f_v = (vx_float64)cr - 128;
    vx_float64 f_r = f_y + 0.0000f*f_u + 1.5748f*f_v;
    vx_float64 f_g = f_y - 0.1873f*f_u - 0.4681f*f_v;
    vx_float64 f_b = f_y + 1.8556f*f_u + 0.0000f*f_v;
    vx_int32 i_r = (vx_int32)f_r;
    vx_int32 i_g = (vx_int32)f_g;
    vx_int32 i_b = (vx_int32)f_b;
    *r = usat8(i_r);
    *g = usat8(i_g);
    *b = usat8(i_b);
}

static void rgb2yuv_bt709(vx_uint8 r, vx_uint8 g, vx_uint8 b,
                          vx_uint8 *y, vx_uint8 *cb, vx_uint8 *cr)
{
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    U'=-0.1146*R' - 0.3854*G' + 0.5000*B'
    V'= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */
    vx_float64 f_r = (vx_float64)r;
    vx_float64 f_g = (vx_float64)g;
    vx_float64 f_b = (vx_float64)b;
    vx_float64 f_y = 0 + 0.2126f*f_r + 0.7152f*f_g + 0.0722f*f_b;
    vx_float64 f_u = 0 - 0.1146f*f_r - 0.3854f*f_g + 0.5000f*f_b;
    vx_float64 f_v = 0 + 0.5000f*f_r - 0.4542f*f_g - 0.0458f*f_b;
    vx_int32 i_y = (vx_int32)f_y;
    vx_int32 i_u = (vx_int32)f_u + 128;
    vx_int32 i_v = (vx_int32)f_v + 128;
    *y  = usat8(i_y);
    *cb = usat8(i_u);
    *cr = usat8(i_v);
}

static void yuv2yuv_601to709(vx_uint8 y0, vx_uint8 cb0, vx_uint8 cr0,
                             vx_uint8 *y1, vx_uint8 *cb1, vx_uint8 *cr1)
{
    /*
    Y' = 1.0090*Y - 0.11826430*Cb - 0.2000311*Cr
    Cb'= 0.0000*Y + 1.01911200*Cb + 0.1146035*Cr
    Cr'= 0.0001*Y + 0.07534570*Cb + 1.0290932*Cr
    */
    vx_float64 f_y0 = (vx_float64)y0;
    vx_float64 f_cb0 = (vx_float64)cb0;
    vx_float64 f_cr0 = (vx_float64)cr0;
    vx_float64 f_y1  = 1.0090*f_y0 - 0.11826430*f_cb0 - 0.2000311*f_cr0;
    vx_float64 f_cb1 = 0.0000*f_y0 + 1.01911200*f_cb0 + 0.1146035*f_cr0;
    vx_float64 f_cr1 = 0.0001*f_y0 + 0.07534570*f_cb0 + 1.0290932*f_cr0;
    vx_int32 i_y = (vx_int32)f_y1;
    vx_int32 i_cb = (vx_int32)f_cb1;
    vx_int32 i_cr = (vx_int32)f_cr1;
    *y1 = usat8(i_y);
    *cb1 = usat8(i_cb);
    *cr1 = usat8(i_cr);
}

// kernel --------------------------------------------------------------------

// nodeless version of the ConvertColor kernel
vx_status vxConvertColor(vx_image src, vx_image dst)
{
    vx_imagepatch_addressing_t src_addr[4], dst_addr[4];
    void *src_base[4] = {NULL};
    void *dst_base[4] = {NULL};
    vx_uint32 y, x, p;
    vx_df_image src_format, dst_format;
    vx_size src_planes, dst_planes;
    vx_enum src_space;
    vx_rectangle_t rect;

    vx_status status = VX_SUCCESS;
    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    status |= vxQueryImage(src, VX_IMAGE_PLANES, &src_planes, sizeof(src_planes));
    status |= vxQueryImage(dst, VX_IMAGE_PLANES, &dst_planes, sizeof(dst_planes));
    status |= vxQueryImage(src, VX_IMAGE_SPACE, &src_space, sizeof(src_space));
    status = vxGetValidRegionImage(src, &rect);
    for (p = 0; p < src_planes; p++)
    {
        status |= vxAccessImagePatch(src, &rect, p, &src_addr[p], &src_base[p], VX_READ_ONLY);
        ownPrintImageAddressing(&src_addr[p]);
    }
    for (p = 0; p < dst_planes; p++)
    {
        status |= vxAccessImagePatch(dst, &rect, p, &dst_addr[p], &dst_base[p], VX_WRITE_ONLY);
        ownPrintImageAddressing(&dst_addr[p]);
    }
    if (status != VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to setup images in Color Convert!\n");
    }

    if ((src_format == VX_DF_IMAGE_RGB) || (src_format == VX_DF_IMAGE_RGBX))
    {
        if (dst_format == VX_DF_IMAGE_RGB || dst_format == VX_DF_IMAGE_RGBX)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *src_rgb = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *dst_rgb = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    dst_rgb[0] = src_rgb[0];
                    dst_rgb[1] = src_rgb[1];
                    dst_rgb[2] = src_rgb[2];
                    if (dst_format == VX_DF_IMAGE_RGBX)
                        dst_rgb[3] = 255;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            vx_uint8 cb[4];
            vx_uint8 cr[4];
            for (y = 0; y < dst_addr[0].dim_y; y+=2)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *rgb[4] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                        vxFormatImagePatchAddress2d(src_base[0], x+1, y, &src_addr[0]),
                                        vxFormatImagePatchAddress2d(src_base[0], x, y+1, &src_addr[0]),
                                        vxFormatImagePatchAddress2d(src_base[0], x+1, y+1, &src_addr[0])};
                    vx_uint8 *luma[4] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y+1, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y+1, &dst_addr[0])};
                    vx_uint8 *cbcr = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    rgb2yuv_bt709(rgb[0][0],rgb[0][1],rgb[0][2],&luma[0][0],&cb[0],&cr[0]);
                    rgb2yuv_bt709(rgb[1][0],rgb[1][1],rgb[1][2],&luma[1][0],&cb[1],&cr[1]);
                    rgb2yuv_bt709(rgb[2][0],rgb[2][1],rgb[2][2],&luma[2][0],&cb[2],&cr[2]);
                    rgb2yuv_bt709(rgb[3][0],rgb[3][1],rgb[3][2],&luma[3][0],&cb[3],&cr[3]);

                    cbcr[0] = (cb[0] + cb[1] + cb[2] + cb[3])/4;
                    cbcr[1] = (cr[0] + cr[1] + cr[2] + cr[3])/4;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *rgb  = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);

                    rgb2yuv_bt709(rgb[0],rgb[1],rgb[2],luma,cb,cr);
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            vx_uint8 cb[4];
            vx_uint8 cr[4];
            for (y = 0; y < dst_addr[0].dim_y; y+=2)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *rgb[4] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                        vxFormatImagePatchAddress2d(src_base[0], x+1, y, &src_addr[0]),
                                        vxFormatImagePatchAddress2d(src_base[0], x, y+1, &src_addr[0]),
                                        vxFormatImagePatchAddress2d(src_base[0], x+1, y+1, &src_addr[0])};

                    vx_uint8 *luma[4] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y+1, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y+1, &dst_addr[0])};

                    vx_uint8 *cbp = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *crp = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);

                    rgb2yuv_bt709(rgb[0][0],rgb[0][1],rgb[0][2],&luma[0][0],&cb[0],&cr[0]);
                    rgb2yuv_bt709(rgb[1][0],rgb[1][1],rgb[1][2],&luma[1][0],&cb[1],&cr[1]);
                    rgb2yuv_bt709(rgb[2][0],rgb[2][1],rgb[2][2],&luma[2][0],&cb[2],&cr[2]);
                    rgb2yuv_bt709(rgb[3][0],rgb[3][1],rgb[3][2],&luma[3][0],&cb[3],&cr[3]);

                    cbp[0] = (uint8_t)(((vx_uint16)cb[0] + (vx_uint16)cb[1] + (vx_uint16)cb[2] + (vx_uint16)cb[3])>>2);
                    crp[0] = (uint8_t)(((vx_uint16)cr[0] + (vx_uint16)cr[1] + (vx_uint16)cr[2] + (vx_uint16)cr[3])>>2);
                }
            }
        }
    }
    else if (src_format == VX_DF_IMAGE_NV21 || src_format == VX_DF_IMAGE_NV12)
    {
        int u_pix = src_format == VX_DF_IMAGE_NV12 ? 0 : 1;
        int v_pix = src_format == VX_DF_IMAGE_NV12 ? 1 : 0;
        if ((dst_format == VX_DF_IMAGE_RGB) || (dst_format == VX_DF_IMAGE_RGBX))
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *crcb = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *rgb = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    if (dst_format == VX_DF_IMAGE_RGBX)
                        rgb[3] = 255;

                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                        yuv2rgb_bt601(luma[0],crcb[u_pix],crcb[v_pix],&rgb[0],&rgb[1],&rgb[2]);
                    else /*if (src_space == VX_COLOR_SPACE_BT709)*/
                        yuv2rgb_bt709(luma[0],crcb[u_pix],crcb[v_pix],&rgb[0],&rgb[1],&rgb[2]);
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12 || dst_format == VX_DF_IMAGE_NV21)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0])};
                    vx_uint8 *cbcr = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *crcb = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    yuv2yuv_601to709(luma[0][0],cbcr[0],cbcr[1],&luma[1][0],&crcb[1],&crcb[0]);
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *crcb = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *yout = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);

                    yout[0] = luma[0];
                    cb[0] = crcb[u_pix];
                    cr[0] = crcb[v_pix];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *crcb = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *yout = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);

                    yout[0] = luma[0];
                    cb[0] = crcb[u_pix];
                    cr[0] = crcb[v_pix];
                }
            }
        }
    }
    else if (src_format == VX_DF_IMAGE_YUYV)
    {
        if (dst_format == VX_DF_IMAGE_RGB)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *yuyv = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *rgb  = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);

                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601(yuyv[0],yuyv[1],yuyv[3],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt601(yuyv[2],yuyv[1],yuyv[3],&rgb[3],&rgb[4],&rgb[5]);
                    }
                    else /*if (src_space == VX_COLOR_SPACE_BT709)*/
                    {
                        yuv2rgb_bt709(yuyv[0],yuyv[1],yuyv[3],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt709(yuyv[2],yuyv[1],yuyv[3],&rgb[3],&rgb[4],&rgb[5]);
                    }
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_RGBX)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *yuyv = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *rgb  = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    rgb[3] = rgb[7] = 255;

                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601(yuyv[0],yuyv[1],yuyv[3],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt601(yuyv[2],yuyv[1],yuyv[3],&rgb[4],&rgb[5],&rgb[6]);
                    }
                    else /*if (src_space == VX_COLOR_SPACE_BT709)*/
                    {
                        yuv2rgb_bt709(yuyv[0],yuyv[1],yuyv[3],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt709(yuyv[2],yuyv[1],yuyv[3],&rgb[4],&rgb[5],&rgb[6]);
                    }
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            for (y = 0; y < dst_addr[0].dim_y; y+=2)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *yuyv[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                          vxFormatImagePatchAddress2d(src_base[0], x, y+1, &src_addr[0])};
                    vx_uint8 *luma[4] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                          vxFormatImagePatchAddress2d(dst_base[0], x+1, y, &dst_addr[0]),
                                          vxFormatImagePatchAddress2d(dst_base[0], x, y+1, &dst_addr[0]),
                                          vxFormatImagePatchAddress2d(dst_base[0], x+1, y+1, &dst_addr[0])};
                    vx_uint8 *cbcr = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    luma[0][0] = yuyv[0][0];
                    luma[1][0] = yuyv[0][2];
                    luma[2][0] = yuyv[1][0];
                    luma[3][0] = yuyv[1][2];
                    cbcr[0] = (yuyv[0][1] + yuyv[1][1])/2;
                    cbcr[1] = (yuyv[0][3] + yuyv[1][3])/2;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *yuyv = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);

                    luma[0] = yuyv[0];
                    luma[1] = yuyv[2];
                    cb[0] = yuyv[1];
                    cr[0] = yuyv[3];
                    cb[1] = yuyv[1];
                    cr[1] = yuyv[3];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            for (y = 0; y < dst_addr[0].dim_y; y+=2)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *yuyv[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                          vxFormatImagePatchAddress2d(src_base[0], x, y+1, &src_addr[0])};
                    vx_uint8 *luma[4] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                          vxFormatImagePatchAddress2d(dst_base[0], x+1, y, &dst_addr[0]),
                                          vxFormatImagePatchAddress2d(dst_base[0], x, y+1, &dst_addr[0]),
                                          vxFormatImagePatchAddress2d(dst_base[0], x+1, y+1, &dst_addr[0])};
                    vx_uint8 *cb = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *cr = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);
                    luma[0][0] = yuyv[0][0];
                    luma[1][0] = yuyv[0][2];
                    luma[2][0] = yuyv[1][0];
                    luma[3][0] = yuyv[1][2];
                    cb[0] = (yuyv[0][1] + yuyv[1][1])/2;
                    cr[0] = (yuyv[0][3] + yuyv[1][3])/2;
                }
            }
        }
    }
    else if (src_format == VX_DF_IMAGE_UYVY)
    {
        if (dst_format == VX_DF_IMAGE_RGB)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *uyvy = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *rgb  = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);

                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601(uyvy[1],uyvy[0],uyvy[2],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt601(uyvy[3],uyvy[0],uyvy[2],&rgb[3],&rgb[4],&rgb[5]);
                    }
                    else /*if (src_space == VX_COLOR_SPACE_BT709)*/
                    {
                        yuv2rgb_bt709(uyvy[1],uyvy[0],uyvy[2],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt709(uyvy[3],uyvy[0],uyvy[2],&rgb[3],&rgb[4],&rgb[5]);
                    }
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_RGBX)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *uyvy = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *rgb  = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    rgb[3] = rgb[7] = 255;

                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                    {
                        yuv2rgb_bt601(uyvy[1],uyvy[0],uyvy[2],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt601(uyvy[3],uyvy[0],uyvy[2],&rgb[4],&rgb[5],&rgb[6]);
                    }
                    else /*if (src_space == VX_COLOR_SPACE_BT709)*/
                    {
                        yuv2rgb_bt709(uyvy[1],uyvy[0],uyvy[2],&rgb[0],&rgb[1],&rgb[2]);
                        yuv2rgb_bt709(uyvy[3],uyvy[0],uyvy[2],&rgb[4],&rgb[5],&rgb[6]);
                    }
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            for (y = 0; y < dst_addr[0].dim_y; y+=2)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *uyvy[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                         vxFormatImagePatchAddress2d(src_base[0], x, y+1, &src_addr[0])};
                    vx_uint8 *luma[4] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y+1, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y+1, &dst_addr[0])};
                    vx_uint8 *cbcr = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    luma[0][0] = uyvy[0][1];
                    luma[1][0] = uyvy[0][3];
                    luma[2][0] = uyvy[1][1];
                    luma[3][0] = uyvy[1][3];
                    cbcr[0] = (uyvy[0][0] + uyvy[1][0])/2;
                    cbcr[1] = (uyvy[0][2] + uyvy[1][2])/2;
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *uyvy = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);

                    luma[0] = uyvy[1];
                    luma[1] = uyvy[3];
                    cb[0] = uyvy[0];
                    cr[0] = uyvy[2];
                    cb[1] = uyvy[0];
                    cr[1] = uyvy[2];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_IYUV)
        {
            for (y = 0; y < dst_addr[0].dim_y; y+=2)
            {
                for (x = 0; x < dst_addr[0].dim_x; x+=2)
                {
                    vx_uint8 *uyvy[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                         vxFormatImagePatchAddress2d(src_base[0], x, y+1, &src_addr[0])};
                    vx_uint8 *luma[4] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y+1, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x+1, y+1, &dst_addr[0])};
                    vx_uint8 *cb = vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1]);
                    vx_uint8 *cr = vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2]);
                    luma[0][0] = uyvy[0][1];
                    luma[1][0] = uyvy[0][3];
                    luma[2][0] = uyvy[1][1];
                    luma[3][0] = uyvy[1][3];
                    cb[0] = (uyvy[0][0] + uyvy[1][0])/2;
                    cr[0] = (uyvy[0][2] + uyvy[1][2])/2;
                }
            }
        }
    }
    else if (src_format == VX_DF_IMAGE_IYUV)
    {
        if (dst_format == VX_DF_IMAGE_RGB || dst_format == VX_DF_IMAGE_RGBX)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(src_base[2], x, y, &src_addr[2]);
                    vx_uint8 *rgb  = vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]);
                    if (dst_format == VX_DF_IMAGE_RGBX)
                        rgb[3] = 255;

                    /*! \todo restricted range 601 ? */
                    if (src_space == VX_COLOR_SPACE_BT601_525 ||
                        src_space == VX_COLOR_SPACE_BT601_625)
                        yuv2rgb_bt601(luma[0],cb[0],cr[0],&rgb[0],&rgb[1],&rgb[2]);
                    else /*if (src_space == VX_COLOR_SPACE_BT709)*/
                        yuv2rgb_bt709(luma[0],cb[0],cr[0],&rgb[0],&rgb[1],&rgb[2]);
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_NV12)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
                    vx_uint8 *cb   = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
                    vx_uint8 *cr   = vxFormatImagePatchAddress2d(src_base[2], x, y, &src_addr[2]);
                    vx_uint8 *nv12[2] = {vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1])};
                    nv12[0][0] = luma[0];
                    nv12[1][0] = cb[0];
                    nv12[1][1] = cr[0];
                }
            }
        }
        else if (dst_format == VX_DF_IMAGE_YUV4)
        {
            for (y = 0; y < dst_addr[0].dim_y; y++)
            {
                for (x = 0; x < dst_addr[0].dim_x; x++)
                {
                    vx_uint8 *luma[2] = {vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                                         vxFormatImagePatchAddress2d(dst_base[0], x, y, &dst_addr[0])};
                    vx_uint8 *cb[2]   = {vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]),
                                         vxFormatImagePatchAddress2d(dst_base[1], x, y, &dst_addr[1])};
                    vx_uint8 *cr[2]   = {vxFormatImagePatchAddress2d(src_base[2], x, y, &src_addr[2]),
                                         vxFormatImagePatchAddress2d(dst_base[2], x, y, &dst_addr[2])};
                    luma[1][0] = luma[0][0];
                    cb[1][0] = cb[0][0];
                    cr[1][0] = cr[0][0];
                }
            }
        }
    }
    status = VX_SUCCESS;
    for (p = 0; p < src_planes; p++)
    {
        status |= vxCommitImagePatch(src, NULL, p, &src_addr[p], src_base[p]);
    }
    for (p = 0; p < dst_planes; p++)
    {
        status |= vxCommitImagePatch(dst, &rect, p, &dst_addr[p], dst_base[p]);
    }
    if (status != VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "Failed to set image patches on source or destination\n");
    }

    return status;
}

