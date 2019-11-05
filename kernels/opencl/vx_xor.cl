
__kernel void vx_xor(int asx, int asy, __global uchar *a, 
                     int bsx, int bsy, __global uchar *b,
                     int csx, int csy, __global uchar *c)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    c[y * csy + x * csx] = a[y * asy + x * asx] ^ b[y * bsy + x * bsx];
}
