//Define 3 types of border
#define VX_ID_KHRONOS 0x000
#define VX_ENUM_BORDER 0x0C
#define VX_ENUM_NONLINEAR 0x16
#define VX_ENUM_BASE(vendor, id)   (((vendor) << 20) | (id << 12))

#define VX_BORDER_UNDEFINED VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x0
#define VX_BORDER_CONSTANT  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x1
#define VX_BORDER_REPLICATE VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_BORDER) + 0x2

#define VX_NONLINEAR_FILTER_MEDIAN  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_NONLINEAR) + 0x0
#define VX_NONLINEAR_FILTER_MIN     VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_NONLINEAR) + 0x1 
#define VX_NONLINEAR_FILTER_MAX     VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_NONLINEAR) + 0x2

uchar min_op(uchar a, uchar b) 
{
    return a < b ? a : b;
}

uchar max_op(uchar a, uchar b) 
{
    return a > b ? a : b;
}

void sort_mid(uchar *a, uchar *b)
{
    const uchar min = min_op(*a, *b);
    const uchar max = max_op(*a, *b);
    
    *a = min;
    *b = max;
}

#define SORT_MID_CROSS_3x3      sort_mid(&pixels[0], &pixels[1]);       \
                                sort_mid(&pixels[2], &pixels[3]);       \
                                sort_mid(&pixels[0], &pixels[2]);       \
                                sort_mid(&pixels[1], &pixels[3]);       \
                                sort_mid(&pixels[1], &pixels[2]);       \
                                sort_mid(&pixels[0], &pixels[4]);       \
                                sort_mid(&pixels[1], &pixels[4]);       \
                                sort_mid(&pixels[2], &pixels[4]);       \

#define SORT_MID_3x3            sort_mid(&pixels[1], &pixels[2]);            \
                                sort_mid(&pixels[4], &pixels[5]);            \
                                sort_mid(&pixels[7], &pixels[8]);            \
                                sort_mid(&pixels[0], &pixels[1]);            \
                                sort_mid(&pixels[3], &pixels[4]);            \
                                sort_mid(&pixels[6], &pixels[7]);            \
                                sort_mid(&pixels[1], &pixels[2]);            \
                                sort_mid(&pixels[4], &pixels[5]);            \
                                sort_mid(&pixels[7], &pixels[8]);            \
                                sort_mid(&pixels[0], &pixels[3]);            \
                                sort_mid(&pixels[5], &pixels[8]);            \
                                sort_mid(&pixels[4], &pixels[7]);            \
                                sort_mid(&pixels[3], &pixels[6]);            \
                                sort_mid(&pixels[1], &pixels[4]);            \
                                sort_mid(&pixels[2], &pixels[5]);            \
                                sort_mid(&pixels[4], &pixels[7]);            \
                                sort_mid(&pixels[4], &pixels[2]);            \
                                sort_mid(&pixels[6], &pixels[4]);            \
                                sort_mid(&pixels[4], &pixels[2]);            \
                                

#define SORT_MID_DISK_5x5       sort_mid(&pixels[0], &pixels[1]);       \          
                                sort_mid(&pixels[2], &pixels[3]);       \
                                sort_mid(&pixels[4], &pixels[5]);       \
                                sort_mid(&pixels[6], &pixels[7]);       \
                                sort_mid(&pixels[8], &pixels[9]);       \
                                sort_mid(&pixels[10], &pixels[11]);     \
                                sort_mid(&pixels[12], &pixels[13]);     \
                                sort_mid(&pixels[14], &pixels[15]);     \
                                sort_mid(&pixels[16], &pixels[17]);     \
                                sort_mid(&pixels[18], &pixels[19]);     \
                                sort_mid(&pixels[0], &pixels[2]);       \
                                sort_mid(&pixels[1], &pixels[3]);       \
                                sort_mid(&pixels[4], &pixels[6]);       \
                                sort_mid(&pixels[5], &pixels[7]);       \
                                sort_mid(&pixels[8], &pixels[10]);      \
                                sort_mid(&pixels[9], &pixels[11]);      \
                                sort_mid(&pixels[12], &pixels[14]);     \
                                sort_mid(&pixels[13], &pixels[15]);     \
                                sort_mid(&pixels[16], &pixels[18]);     \
                                sort_mid(&pixels[17], &pixels[19]);     \
                                sort_mid(&pixels[1], &pixels[2]);       \
                                sort_mid(&pixels[5], &pixels[6]);       \
                                sort_mid(&pixels[0], &pixels[4]);       \
                                sort_mid(&pixels[3], &pixels[7]);       \
                                sort_mid(&pixels[9], &pixels[10]);      \
                                sort_mid(&pixels[13], &pixels[14]);     \
                                sort_mid(&pixels[8], &pixels[12]);      \
                                sort_mid(&pixels[11], &pixels[15]);     \
                                sort_mid(&pixels[17], &pixels[18]);     \
                                sort_mid(&pixels[16], &pixels[20]);     \
                                sort_mid(&pixels[1], &pixels[5]);       \
                                sort_mid(&pixels[2], &pixels[6]);       \
                                sort_mid(&pixels[9], &pixels[13]);      \
                                sort_mid(&pixels[10], &pixels[14]);     \
                                sort_mid(&pixels[0], &pixels[8]);       \
                                sort_mid(&pixels[7], &pixels[15]);      \
                                sort_mid(&pixels[17], &pixels[20]);     \
                                sort_mid(&pixels[1], &pixels[4]);       \
                                sort_mid(&pixels[3], &pixels[6]);       \
                                sort_mid(&pixels[9], &pixels[12]);      \
                                sort_mid(&pixels[11], &pixels[14]);     \
                                sort_mid(&pixels[18], &pixels[20]);     \
                                sort_mid(&pixels[0], &pixels[16]);      \
                                sort_mid(&pixels[2], &pixels[4]);       \
                                sort_mid(&pixels[3], &pixels[5]);       \
                                sort_mid(&pixels[10], &pixels[12]);     \
                                sort_mid(&pixels[11], &pixels[13]);     \
                                sort_mid(&pixels[1], &pixels[9]);       \
                                sort_mid(&pixels[6], &pixels[14]);      \
                                sort_mid(&pixels[19], &pixels[20]);     \
                                sort_mid(&pixels[3], &pixels[4]);       \
                                sort_mid(&pixels[11], &pixels[12]);     \
                                sort_mid(&pixels[1], &pixels[8]);       \
                                sort_mid(&pixels[2], &pixels[10]);      \
                                sort_mid(&pixels[5], &pixels[13]);      \
                                sort_mid(&pixels[7], &pixels[14]);      \
                                sort_mid(&pixels[3], &pixels[11]);      \
                                sort_mid(&pixels[2], &pixels[8]);       \
                                sort_mid(&pixels[4], &pixels[12]);      \
                                sort_mid(&pixels[7], &pixels[13]);      \
                                sort_mid(&pixels[1], &pixels[17]);      \
                                sort_mid(&pixels[3], &pixels[10]);      \
                                sort_mid(&pixels[5], &pixels[12]);      \
                                sort_mid(&pixels[1], &pixels[16]);      \
                                sort_mid(&pixels[2], &pixels[18]);      \
                                sort_mid(&pixels[3], &pixels[9]);       \
                                sort_mid(&pixels[6], &pixels[12]);      \
                                sort_mid(&pixels[2], &pixels[16]);      \
                                sort_mid(&pixels[3], &pixels[8]);       \
                                sort_mid(&pixels[7], &pixels[12]);      \
                                sort_mid(&pixels[5], &pixels[9]);       \
                                sort_mid(&pixels[6], &pixels[10]);      \
                                sort_mid(&pixels[4], &pixels[8]);       \
                                sort_mid(&pixels[7], &pixels[11]);      \
                                sort_mid(&pixels[3], &pixels[19]);      \
                                sort_mid(&pixels[5], &pixels[8]);       \
                                sort_mid(&pixels[7], &pixels[10]);      \
                                sort_mid(&pixels[3], &pixels[18]);      \
                                sort_mid(&pixels[4], &pixels[20]);      \
                                sort_mid(&pixels[6], &pixels[8]);       \
                                sort_mid(&pixels[7], &pixels[9]);       \
                                sort_mid(&pixels[3], &pixels[17]);      \
                                sort_mid(&pixels[5], &pixels[20]);      \
                                sort_mid(&pixels[7], &pixels[8]);       \
                                sort_mid(&pixels[3], &pixels[16]);      \
                                sort_mid(&pixels[6], &pixels[20]);      \
                                sort_mid(&pixels[5], &pixels[17]);      \
                                sort_mid(&pixels[7], &pixels[20]);      \
                                sort_mid(&pixels[4], &pixels[16]);      \
                                sort_mid(&pixels[6], &pixels[18]);      \
                                sort_mid(&pixels[5], &pixels[16]);      \
                                sort_mid(&pixels[7], &pixels[19]);      \
                                sort_mid(&pixels[7], &pixels[18]);      \   
                                sort_mid(&pixels[6], &pixels[16]);      \
                                sort_mid(&pixels[7], &pixels[17]);      \
                                sort_mid(&pixels[10], &pixels[18]);     \
                                sort_mid(&pixels[7], &pixels[16]);      \   
                                sort_mid(&pixels[9], &pixels[17]);      \
                                sort_mid(&pixels[8], &pixels[16]);      \
                                sort_mid(&pixels[9], &pixels[16]);      \
                                sort_mid(&pixels[10], &pixels[16]);     \
                                
                                
                                
 #define SORT_MID_BOX_5x5       sort_mid(&pixels[1], &pixels[2]);       \
                                sort_mid(&pixels[0], &pixels[1]);       \
                                sort_mid(&pixels[1], &pixels[2]);       \
                                sort_mid(&pixels[4], &pixels[5]);       \
                                sort_mid(&pixels[3], &pixels[4]);       \
                                sort_mid(&pixels[4], &pixels[5]);       \
                                sort_mid(&pixels[0], &pixels[3]);       \
                                sort_mid(&pixels[2], &pixels[5]);       \
                                sort_mid(&pixels[2], &pixels[3]);       \
                                sort_mid(&pixels[1], &pixels[4]);       \   
                                sort_mid(&pixels[1], &pixels[2]);       \
                                sort_mid(&pixels[3], &pixels[4]);       \
                                sort_mid(&pixels[7], &pixels[8]);       \
                                sort_mid(&pixels[6], &pixels[7]);       \
                                sort_mid(&pixels[7], &pixels[8]);       \
                                sort_mid(&pixels[10], &pixels[11]);     \
                                sort_mid(&pixels[9], &pixels[10]);      \
                                sort_mid(&pixels[10], &pixels[11]);     \
                                sort_mid(&pixels[6], &pixels[9]);       \
                                sort_mid(&pixels[8], &pixels[11]);      \
                                sort_mid(&pixels[8], &pixels[9]);       \
                                sort_mid(&pixels[7], &pixels[10]);      \
                                sort_mid(&pixels[7], &pixels[8]);       \
                                sort_mid(&pixels[9], &pixels[10]);      \
                                sort_mid(&pixels[0], &pixels[6]);       \
                                sort_mid(&pixels[4], &pixels[10]);      \
                                sort_mid(&pixels[4], &pixels[6]);       \
                                sort_mid(&pixels[2], &pixels[8]);       \   
                                sort_mid(&pixels[2], &pixels[4]);       \
                                sort_mid(&pixels[6], &pixels[8]);       \
                                sort_mid(&pixels[1], &pixels[7]);       \
                                sort_mid(&pixels[5], &pixels[11]);      \
                                sort_mid(&pixels[5], &pixels[7]);       \
                                sort_mid(&pixels[3], &pixels[9]);       \
                                sort_mid(&pixels[3], &pixels[5]);       \   
                                sort_mid(&pixels[7], &pixels[9]);       \
                                sort_mid(&pixels[1], &pixels[2]);       \
                                sort_mid(&pixels[3], &pixels[4]);       \   
                                sort_mid(&pixels[5], &pixels[6]);       \   
                                sort_mid(&pixels[7], &pixels[8]);       \
                                sort_mid(&pixels[9], &pixels[10]);      \
                                sort_mid(&pixels[13], &pixels[14]);     \
                                sort_mid(&pixels[12], &pixels[13]);     \
                                sort_mid(&pixels[13], &pixels[14]);     \
                                sort_mid(&pixels[16], &pixels[17]);     \
                                sort_mid(&pixels[15], &pixels[16]);     \
                                sort_mid(&pixels[16], &pixels[17]);     \
                                sort_mid(&pixels[12], &pixels[15]);     \
                                sort_mid(&pixels[14], &pixels[17]);     \
                                sort_mid(&pixels[14], &pixels[15]);     \
                                sort_mid(&pixels[13], &pixels[16]);     \   
                                sort_mid(&pixels[13], &pixels[14]);     \
                                sort_mid(&pixels[15], &pixels[16]);     \
                                sort_mid(&pixels[19], &pixels[20]);     \
                                sort_mid(&pixels[18], &pixels[19]);     \
                                sort_mid(&pixels[19], &pixels[20]);     \
                                sort_mid(&pixels[21], &pixels[22]);     \
                                sort_mid(&pixels[23], &pixels[24]);     \
                                sort_mid(&pixels[21], &pixels[23]);     \
                                sort_mid(&pixels[22], &pixels[24]);     \
                                sort_mid(&pixels[22], &pixels[23]);     \
                                sort_mid(&pixels[18], &pixels[21]);     \
                                sort_mid(&pixels[20], &pixels[23]);     \
                                sort_mid(&pixels[20], &pixels[21]);     \
                                sort_mid(&pixels[19], &pixels[22]);     \
                                sort_mid(&pixels[22], &pixels[24]);     \
                                sort_mid(&pixels[19], &pixels[20]);     \
                                sort_mid(&pixels[21], &pixels[22]);     \
                                sort_mid(&pixels[23], &pixels[24]);     \
                                sort_mid(&pixels[12], &pixels[18]);     \
                                sort_mid(&pixels[16], &pixels[22]);     \
                                sort_mid(&pixels[16], &pixels[18]);     \
                                sort_mid(&pixels[14], &pixels[20]);     \
                                sort_mid(&pixels[20], &pixels[24]);     \
                                sort_mid(&pixels[14], &pixels[16]);     \
                                sort_mid(&pixels[18], &pixels[20]);     \
                                sort_mid(&pixels[22], &pixels[24]);     \
                                sort_mid(&pixels[13], &pixels[19]);     \
                                sort_mid(&pixels[17], &pixels[23]);     \
                                sort_mid(&pixels[17], &pixels[19]);     \
                                sort_mid(&pixels[15], &pixels[21]);     \
                                sort_mid(&pixels[15], &pixels[17]);     \
                                sort_mid(&pixels[19], &pixels[21]);     \
                                sort_mid(&pixels[13], &pixels[14]);     \
                                sort_mid(&pixels[15], &pixels[16]);     \
                                sort_mid(&pixels[17], &pixels[18]);     \
                                sort_mid(&pixels[19], &pixels[20]);     \
                                sort_mid(&pixels[21], &pixels[22]);     \
                                sort_mid(&pixels[23], &pixels[24]);     \
                                sort_mid(&pixels[0], &pixels[12]);      \
                                sort_mid(&pixels[8], &pixels[20]);      \
                                sort_mid(&pixels[8], &pixels[12]);      \
                                sort_mid(&pixels[4], &pixels[16]);      \
                                sort_mid(&pixels[16], &pixels[24]);     \
                                sort_mid(&pixels[12], &pixels[16]);     \
                                sort_mid(&pixels[2], &pixels[14]);      \
                                sort_mid(&pixels[10], &pixels[22]);     \
                                sort_mid(&pixels[10], &pixels[14]);     \
                                sort_mid(&pixels[6], &pixels[18]);      \
                                sort_mid(&pixels[6], &pixels[10]);      \
                                sort_mid(&pixels[10], &pixels[12]);     \
                                sort_mid(&pixels[1], &pixels[13]);      \
                                sort_mid(&pixels[9], &pixels[21]);      \
                                sort_mid(&pixels[9], &pixels[13]);      \
                                sort_mid(&pixels[5], &pixels[17]);      \
                                sort_mid(&pixels[13], &pixels[17]);     \
                                sort_mid(&pixels[3], &pixels[15]);      \
                                sort_mid(&pixels[11], &pixels[23]);     \
                                sort_mid(&pixels[11], &pixels[15]);     \
                                sort_mid(&pixels[7], &pixels[19]);      \
                                sort_mid(&pixels[7], &pixels[11]);      \
                                sort_mid(&pixels[11], &pixels[13]);     \
                                sort_mid(&pixels[11], &pixels[12]);     \

                         
#define FILTER_VALUE_3x3       switch (function)                                            \
                               {                                                            \
                                   case VX_NONLINEAR_FILTER_MIN:                            \
                                   {                                                        \
                                       min_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           min_value = min_op(min_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = min_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MAX:                            \
                                   {                                                        \
                                       max_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           max_value = max_op(max_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = max_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MEDIAN:                         \
                                   {                                                        \
                                       SORT_MID_3x3;                                        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = pixels[4];                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                }                                                           \
                                                            
                            
#define FILTER_CROSS_3x3       switch (function)                                            \
                               {                                                            \
                                   case VX_NONLINEAR_FILTER_MIN:                            \
                                   {                                                        \
                                       min_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           min_value = min_op(min_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = min_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MAX:                            \
                                   {                                                        \
                                       max_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           max_value = max_op(max_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = max_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MEDIAN:                         \
                                   {                                                        \
                                       SORT_MID_CROSS_3x3;                                  \
                                                                                            \
                                       dst[x * dsx + y * dsy] = pixels[2];                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                }                                                           \
                                
                                
#define FILTER_DISK_5x5        switch (function)                                            \
                               {                                                            \
                                   case VX_NONLINEAR_FILTER_MIN:                            \
                                   {                                                        \
                                       min_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           min_value = min_op(min_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = min_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MAX:                            \
                                   {                                                        \
                                       max_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           max_value = max_op(max_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = max_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MEDIAN:                         \
                                   {                                                        \
                                       SORT_MID_DISK_5x5;                                   \
                                                                                            \
                                       dst[x * dsx + y * dsy] = pixels[10];                 \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                }                                                           \
                                
#define FILTER_BOX_5x5         switch (function)                                            \
                               {                                                            \
                                   case VX_NONLINEAR_FILTER_MIN:                            \
                                   {                                                        \
                                       min_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           min_value = min_op(min_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = min_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MAX:                            \
                                   {                                                        \
                                       max_value = pixels[0];                               \
                                       for (i = 1; i < count_mask; i++)                     \
                                           max_value = max_op(max_value, pixels[i]);        \
                                                                                            \
                                       dst[x * dsx + y * dsy] = max_value;                  \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                   case VX_NONLINEAR_FILTER_MEDIAN:                         \
                                   {                                                        \
                                       SORT_MID_BOX_5x5;                                    \
                                                                                            \
                                       dst[x * dsx + y * dsy] = pixels[12];                 \
                                                                                            \
                                       break;                                               \
                                   }                                                        \
                                }                                                           \


__kernel void vx_nonlinearfilter(uint function, int ssx, int ssy, __global uchar *src,
                                 __global uchar *mask, uint left, uint top, uint right, uint bottom,
                                 int mat_rows, int count_mask, int bordermode, uchar const_vaule,
                                 int dsx, int dsy, __global uchar *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const size_t high_x = get_global_size(0);
    const size_t high_y = get_global_size(1);
    
    int ky, kx, i;
    uint dest_index = 0;
    uint mask_index = 0;
    uchar pixels[25], min_value, max_value;
    
    if (bordermode == VX_BORDER_CONSTANT)
    {
        for (ky = -(int)top; ky <= (int)bottom; ++ky)
        {
            int yy = y + ky;
            int ccase_y = yy < 0 || yy >= high_y;

            for (kx = -(int)left; kx <= (int)right; ++kx, ++mask_index)
            {
                int xx = x + kx;
                int ccase = ccase_y || xx < 0 || xx >= high_x;

                if (mask[mask_index])
                {
                    if (!ccase)
                        pixels[dest_index++] = src[xx * ssx + yy * ssy];
                    else
                        pixels[dest_index++] = const_vaule;
                }
            }
        }
        
        switch (mat_rows)                       
        {                                       
            case 3 :  //mask = 3x3              
            {                                   
                if (count_mask == 5)            
                {                               
                    FILTER_CROSS_3x3;           
                }                               
                else  //count_mask = 9          
                {                               
                    FILTER_VALUE_3x3;           
                }                               
                break;                          
            }                                   
            case 5 :  //mask = 5x5              
            {                                   
                if (count_mask == 9)            
                {                               
                    FILTER_VALUE_3x3;           
                }                               
                else if (count_mask == 21)      
                {                               
                    FILTER_DISK_5x5;            
                }                               
                else   //count_mask = 25        
                {                               
                    FILTER_BOX_5x5;             
                }                               
                break;                          
            }                                   
        }                                       
    }
    else
    {
        for (ky = -(int)top; ky <= (int)bottom; ++ky)
        {
            int yy = y + ky;
            yy = yy < 0 ? 0 : yy >= high_y ? high_y - 1 : yy;

            for (kx = -(int)left; kx <= (int)right; ++kx, ++mask_index)
            {
                int xx = x + kx;
                xx = xx < 0 ? 0 : xx >= high_x ? high_x - 1 : xx;
                if (mask[mask_index])
                    pixels[dest_index++] = src[xx * ssx + yy * ssy];
            }
        }
        
        switch (mat_rows)                       
        {                                       
            case 3 :  //mask = 3x3              
            {                                   
                if (count_mask == 5)            
                {                               
                    FILTER_CROSS_3x3;           
                }                               
                else  //count_mask = 9          
                {                               
                    FILTER_VALUE_3x3;           
                }                               
                break;                          
            }                                   
            case 5 :  //mask = 5x5              
            {                                   
                if (count_mask == 9)            
                {                               
                    FILTER_VALUE_3x3;           
                }                               
                else if (count_mask == 21)      
                {                               
                    FILTER_DISK_5x5;            
                }                               
                else   //count_mask = 25        
                {                               
                    FILTER_BOX_5x5;             
                }                               
                break;                          
            }                                   
        }  
    }
}