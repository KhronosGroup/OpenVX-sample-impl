//Define 3 types of border
#define VX_ID_KHRONOS 0x000
#define VX_ENUM_BORDER 0x0C
#define VX_ENUM_BASE(vendor, id)   (((vendor) << 20) | (id << 12))

#define VX_BORDER_UNDEFINED VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x0
#define VX_BORDER_CONSTANT  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x1
#define VX_BORDER_REPLICATE VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x2

uchar min_op(uchar a, uchar b) 
{
    return a < b ? a : b;
}

#define ERODE3x3    pixels[0] = src[x_top * ssx + y_top * ssy];     \
                    pixels[1] = src[x     * ssx + y_top * ssy];     \
                    pixels[2] = src[x_bot * ssx + y_top * ssy];     \
                    pixels[3] = src[x_top * ssx + y     * ssy];     \
                    pixels[4] = src[x     * ssx + y     * ssy];     \
                    pixels[5] = src[x_bot * ssx + y     * ssy];     \
                    pixels[6] = src[x_top * ssx + y_bot * ssy];     \
                    pixels[7] = src[x     * ssx + y_bot * ssy];     \
                    pixels[8] = src[x_bot * ssx + y_bot * ssy];     \
                    min_value = pixels[0];                          \
                    for (i = 1; i < 9; i++)                         \
                        min_value = min_op(min_value, pixels[i]);   \
                    dst[x * dsx + y * dsy] = min_value;             \

__kernel void vx_erode3x3(int ssx, int ssy, __global uchar *src,
                          int bordermode, uchar const_vaule,
                          int dsx, int dsy, __global uchar *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const size_t high_x = get_global_size(0);
    const size_t high_y = get_global_size(1);
    uint sum = 0;
    
    int y_top = y - 1;
    int y_bot = y + 1;
    int x_top = x - 1;
    int x_bot = x + 1;
    
    int ky, kx, i;
    uint dest_index = 0;
    uchar pixels[9], min_value;
    
    if (bordermode == VX_BORDER_CONSTANT)
    {
        // Calculate border
        if (y == 0 || x == 0 || x == high_x - 1 || y == high_y - 1)
        {           
            for (ky = -1; ky <= 1; ++ky)
            {
                int yy = y + ky;
                int ccase_y = yy < 0 || yy >= high_y;

                for (kx = -1; kx <= 1; ++kx, ++dest_index)
                {
                    int xx = x + kx;
                    int ccase = ccase_y || xx < 0 || xx >= high_x;

                    if (!ccase)
                        pixels[dest_index] = src[xx * ssx + yy * ssy];
                    else
                        pixels[dest_index] = const_vaule;
                }
            }

            min_value = pixels[0];
            for (i = 1; i < 9; i++)
                min_value = min_op(min_value, pixels[i]);

            dst[x * dsx + y * dsy] = min_value;  
        }
        else
        {
            ERODE3x3;
        }
    }
    else
    {
        if (bordermode == VX_BORDER_REPLICATE)
        {
            y_top = y_top < 0 ? 0 : y - 1;
            y_bot = y_bot >= high_y ? high_y - 1 : y + 1;
            x_top = x_top < 0 ? 0 : x - 1;
            x_bot = x_bot >= high_x ? high_x - 1 : x + 1;
        }

        ERODE3x3;
    }
}
