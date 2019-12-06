
//Define 3 types of border
#define VX_ID_KHRONOS 0x000
#define VX_ENUM_BORDER 0x0C
#define VX_ENUM_BASE(vendor, id)   (((vendor) << 20) | (id << 12))

#define VX_BORDER_UNDEFINED VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x0
#define VX_BORDER_CONSTANT  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x1
#define VX_BORDER_REPLICATE VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x2

#define GAUSSIAN3x3     sum += (uint)src[x_top * ssx + y_top * ssy];   \
                        sum += 2*(uint)src[x     * ssx + y_top * ssy]; \
                        sum += (uint)src[x_bot * ssx + y_top * ssy];   \
                        sum += 2*(uint)src[x_top * ssx + y     * ssy]; \
                        sum += 4*(uint)src[x     * ssx + y     * ssy]; \
                        sum += 2*(uint)src[x_bot * ssx + y     * ssy]; \
                        sum += (uint)src[x_top * ssx + y_bot * ssy];   \
                        sum += 2*(uint)src[x     * ssx + y_bot * ssy]; \
                        sum += (uint)src[x_bot * ssx + y_bot * ssy];   \
                        sum = sum / 16;                                \
                        dst[x * dsx + y * dsy] = (uchar)sum;           \

__kernel void vx_gaussian3x3(int ssx, int ssy, __global uchar *src,
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

            sum = pixel[0] + 2*pixel[1] + pixel[2] + 2*pixel[3] + 4*pixel[4] + 2*pixel[5] + pixel[6] + 2*pixel[7] + pixel[8];

            sum = sum / 16;
            dst[x * dsx + y * dsy] = (uchar)sum;
        }
        else
        {
            GAUSSIAN3x3;
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

        GAUSSIAN3x3;
    }
}
