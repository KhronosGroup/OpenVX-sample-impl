
#if 1

__kernel void vx_absdiff(int ssx0, int ssy0, __global uchar *src0,
                         int ssx1, int ssy1, __global uchar *src1,
                         int dsx,  int dsy,  __global uchar *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    uchar src_value0 = src0[x * ssx0 + y * ssy0];
    uchar src_value1 = src1[x * ssx1 + y * ssy1];
   
    if (src_value0 > src_value1)
        dst[x * dsx + y * dsy] = src_value0 - src_value1;
    else
        dst[x * dsx + y * dsy] = src_value1 - src_value0;
}

#else 

__kernel void vx_absdiff(int ssx0, int ssy0, __global short *src0,
                         int ssx1, int ssy1, __global short *src1,
                         int dsx,  int dsy,  __global short *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    
    short src_value0 = src0[x * ssx0 / 2 + y * ssy0 / 2];
    short src_value1 = src1[x * ssx1 / 2 + y * ssy1 / 2];
    uint val;
   
    if (src_value0 > src_value1)
        val = src_value0 - src_value1;
    else
        val = src_value1 - src_value0;
        
    dst[x * dsx / 2 + y * dsy / 2] = (short)((val > 32767) ? 32767 : val);
}

#endif