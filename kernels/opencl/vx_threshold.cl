
#define VX_ID_KHRONOS 0x000
#define VX_ENUM_THRESHOLD_TYPE 0x0B
#define VX_ENUM_BASE(vendor, id)   (((vendor) << 20) | (id << 12))

#define VX_THRESHOLD_TYPE_BINARY  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_THRESHOLD_TYPE) + 0x0
#define VX_THRESHOLD_TYPE_RANGE   VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_THRESHOLD_TYPE) + 0x1

#define VX_DF_IMAGE(a,b,c,d)                  ((a) | (b << 8) | (c << 16) | (d << 24))
#define VX_DF_IMAGE_U8   VX_DF_IMAGE('U','0','0','8')
#define VX_DF_IMAGE_S16  VX_DF_IMAGE('S','0','1','6')

#if 0

__kernel void vx_threshold(int ssx, int ssy, __global uchar *src,
                           int type, uchar value, uchar lower, uchar upper,
                           int dsx, int dsy, __global uchar *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    uchar true_value = 255;
    uchar false_value = 0;

    uchar src_value = src[x * ssx + y * ssy];
   
    if (type == VX_THRESHOLD_TYPE_BINARY)
    {
        if (src_value > value)
        {
            dst[x * dsx + y * dsy] = true_value;
        }
        else
        {
            dst[x * dsx + y * dsy] = false_value;
        }
    }

    if (type == VX_THRESHOLD_TYPE_RANGE)
    {
        if (src_value > upper)
        {
            dst[x * dsx + y * dsy] = false_value;
        }
        else if (src_value < lower)
        {
            dst[x * dsx + y * dsy] = false_value;
        }
        else
        {
            dst[x * dsx + y * dsy] = true_value;
        }
    }
}

#else 

__kernel void vx_threshold(int ssx, int ssy, __global short *src,
                           int type, short value, short lower, short upper,
                           int dsx, int dsy, __global uchar *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    uchar true_value = 255;
    uchar false_value = 0;

    short src_value = src[x * ssx / 2 + y * ssy / 2];
   
    if (type == VX_THRESHOLD_TYPE_BINARY)
    {
        if (src_value > value)
        {
            dst[x * dsx + y * dsy] = true_value;
        }
        else
        {
            dst[x * dsx + y * dsy] = false_value;
        }
    }

    if (type == VX_THRESHOLD_TYPE_RANGE)
    {
        if (src_value > upper)
        {
            dst[x * dsx + y * dsy] = false_value;
        }
        else if (src_value < lower)
        {
            dst[x * dsx + y * dsy] = false_value;
        }
        else
        {
            dst[x * dsx + y * dsy] = true_value;
        }
    }
}

#endif