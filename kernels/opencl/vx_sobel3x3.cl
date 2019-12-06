
//Define 3 types of border
#define VX_ID_KHRONOS 0x000
#define VX_ENUM_BORDER 0x0C
#define VX_ENUM_BASE(vendor, id)   (((vendor) << 20) | (id << 12))

#define VX_BORDER_UNDEFINED VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x0
#define VX_BORDER_CONSTANT  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x1
#define VX_BORDER_REPLICATE VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x2

#define SOBEL3x3_gx     sx -= (uint)src[x_top * ssx + y_top * ssy];     \
                        sx -= 2 * (uint)src[x_top * ssx + y * ssy];     \
                        sx -= (uint)src[x_top * ssx + y_bot * ssy];     \
                        sx += (uint)src[x_bot * ssx + y_top * ssy];     \
                        sx += 2 * (uint)src[x_bot * ssx + y * ssy];     \
                        sx += (uint)src[x_bot * ssx + y_bot * ssy];     \
                        gx[x * dsx1 + y * dsy1] = (short)sx;            \
                        
#define SOBEL3x3_gy     sy -= (uint)src[x_top * ssx + y_top * ssy];     \
                        sy -= 2 * (uint)src[x * ssx + y_top * ssy];     \
                        sy -= (uint)src[x_bot * ssx + y_top * ssy];     \
                        sy += (uint)src[x_top * ssx + y_bot * ssy];     \
                        sy += 2 * (uint)src[x * ssx + y_bot * ssy];     \
                        sy += (uint)src[x_bot * ssx + y_bot * ssy];     \
                        gy[x * dsx2 + y * dsy2] = (short)sy;            \


__kernel void vx_sobel3x3(int ssx, int ssy, __global uchar *src,
                          int bordermode, uchar const_vaule,
                          int dsx1, int dsy1, __global short *gx,
                          int dsx2, int dsy2, __global short *gy)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const size_t high_x = get_global_size(0);
    const size_t high_y = get_global_size(1);
    int sx = 0, sy = 0;
    
    int y_top = y - 1;
    int y_bot = y + 1;
    int x_top = x - 1;
    int x_bot = x + 1;
    
    int ky, kx;
    uint dest_index = 0;
    
    if (bordermode == VX_BORDER_CONSTANT)
    {
        uchar pixel[9];
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
                        pixel[dest_index] = src[xx * ssx + yy * ssy];
                    else
                        pixel[dest_index] = const_vaule;
                }
            }

            if (gx)
            {
                sx = pixel[8] + 2*pixel[5] - pixel[6] - pixel[0] - 2*pixel[3] + pixel[2];

                gx[x * dsx1 + y * dsy1] = (short)sx;
            }
            if (gy)
            {
                sy = pixel[6] + 2*pixel[7] + pixel[8] - pixel[0] - 2*pixel[1] - pixel[2];

                gy[x * dsx2 + y * dsy2] = (short)sy;
            }
        }
        else
        {
            if (gx)
            {
                SOBEL3x3_gx;
            }
            if (gy)
            {
                SOBEL3x3_gy;
            }
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

        if (gx)
        {
            SOBEL3x3_gx;
        }
        if (gy)
        {
            SOBEL3x3_gy;
        }
    }
}
