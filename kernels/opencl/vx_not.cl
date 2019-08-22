
__kernel void vx_not(int asx, int asy, __global uchar *a, 
                     int bsx, int bsy, __global uchar *b)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    b[y * bsy + x * bsx] = ~a[y * asy + x * asx];
}
