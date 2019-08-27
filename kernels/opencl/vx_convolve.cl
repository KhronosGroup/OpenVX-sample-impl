
//Define 3 types of border
#define VX_ID_KHRONOS 0x000
#define VX_ENUM_BORDER 0x0C
#define VX_ENUM_BASE(vendor, id)   (((vendor) << 20) | (id << 12))

#define VX_BORDER_UNDEFINED VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x0
#define VX_BORDER_CONSTANT  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x1
#define VX_BORDER_REPLICATE VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x2

#define C_MAX_CONVOLUTION_DIM (15)
#define UINT8_MAX        255

#define Convolve                                                                            \
    uchar slice[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };                     \
    uint center_x = x, center_y = y;                                                        \
    int width = high_x, height = high_y;                                                    \
    int ky, kx;                                                                             \
    uint dest_index = 0;                                                                    \
                                                                                            \
    if( bordermode == VX_BORDER_REPLICATE || bordermode == VX_BORDER_UNDEFINED )            \
    {                                                                                       \
        for (ky = -(int)conv_radius_y; ky <= (int)conv_radius_y; ++ky)                      \
        {                                                                                   \
            int yy = (int)(center_y + ky);                                                  \
            yy = yy < 0 ? 0 : yy >= height ? height - 1 : yy;                               \
                                                                                            \
            for (kx = -(int)conv_radius_x; kx <= (int)conv_radius_x; ++kx, ++dest_index)    \
            {                                                                               \
                int xx = (int)(center_x + kx);                                              \
                xx = xx < 0 ? 0 : xx >= width ? width - 1 : xx;                             \
                slice[dest_index] = src[xx * ssx + yy * ssy];                               \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
    else if( bordermode == VX_BORDER_CONSTANT )                                             \
    {                                                                                       \
        for (ky = -(int)conv_radius_y; ky <= (int)conv_radius_y; ++ky)                      \
        {                                                                                   \
            int yy = (int)(center_y + ky);                                                  \
            int ccase_y = yy < 0 || yy >= height;                                           \
                                                                                            \
            for (kx = -(int)conv_radius_x; kx <= (int)conv_radius_x; ++kx, ++dest_index)    \
            {                                                                               \
                int xx = (int)(center_x + kx);                                              \
                int ccase = ccase_y || xx < 0 || xx >= width;                               \
                if( !ccase )                                                                \
                    slice[dest_index] = src[xx * ssx + yy * ssy];                           \
                else                                                                        \
                    slice[dest_index] = (uchar)const_vaule;                                 \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
                                                                                            \
    for (int i = 0; i < (int)(conv_width * conv_height); ++i)                               \
        sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];                       \
                                                                                            \
    value = sum / (int)scale;                                                               \
                                                                                            \
    if (value < 0)    dst[x * dsx + y * dsy] = 0;                                           \
    else if (value > UINT8_MAX)    dst[x * dsx + y * dsy] = UINT8_MAX;                      \
    else    dst[x * dsx + y * dsy] = value;         

__kernel void vx_Convolve(int ssx, int ssy, __global uchar *src,
                        int bordermode, uchar const_vaule,
						uint conv_width, uint conv_height, 
						uint scale, __global short *conv_mat,
                        int dsx, int dsy, __global uchar *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    int low_x = 0, low_y = 0;
    int high_x = get_global_size(0);
    int high_y = get_global_size(1);
    int sum = 0;
    int value = 0;

    int conv_radius_x, conv_radius_y;
    conv_radius_x = (int)conv_width / 2;
    conv_radius_y = (int)conv_height / 2;
   
    if (bordermode == VX_BORDER_UNDEFINED)
    {
        low_x = conv_radius_x;
        high_x = ((high_x >= (uint)conv_radius_x) ? high_x - conv_radius_x : 0);
        low_y = conv_radius_y;
        high_y = ((high_y >= (uint)conv_radius_y) ? high_y - conv_radius_y : 0);
    }
    
    Convolve;
    
}
