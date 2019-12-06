
#define DBL_EPSILON     2.2204460492503131e-016 

#define M_PI 3.1415926535897932384626433832795

#define ABS(x)    ((x) > 0 ? (x) : -(x))

#define FLOOR(x)    (x > 0 ? (int)(x) : (int)(x - 0.99))

__kernel void vx_phase(int ssx0, int ssy0, __global short *src0,
                       int ssx1, int ssy1, __global short *src1,
                       int dsx,  int dsy,  __global uchar *dst)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    
    float scale = 256.0f / 360.0f;

    float P1 = ((float)( 0.9997878412794807  * (180.0 / M_PI) * scale)), 
    P3 = ((float)(-0.3258083974640975  * (180.0 / M_PI) * scale)),       
    P5 = ((float)( 0.1555786518463281  * (180.0 / M_PI) * scale)),       
    P7 = ((float)(-0.04432655554792128 * (180.0 / M_PI) * scale)),       
    A_90 = ((float)(90.f * scale)),                                      
    A_180 = ((float)(180.f * scale)),                                    
    A_360 = ((float)(360.f * scale));                                   
    
    /* -M_PI to M_PI */
    float val_x;
    float val_y;
    
    val_x = (float)(src0[x * ssx0 / 2 + y * ssy0 / 2]);
    val_y = (float)(src1[x * ssx1 / 2 + y * ssy1 / 2]);
    
    float arct;

    float ax = ABS(val_x), ay = ABS(val_y);                             
    float c, c2;                                                   
    if (ax >= ay)                                                   
    {                                                               
        c = ay / (ax + (float)DBL_EPSILON);                        
        c2 = c * c;                                                 
        arct = (((P7 * c2 + P5) * c2 + P3) * c2 + P1) * c;            
    }                                                              
    else                                                           
    {                                                               
        c = ax / (ay + (float)DBL_EPSILON);                         
        c2 = c * c;                                                 
        arct = A_90 - (((P7 * c2 + P5) * c2 + P3) * c2 + P1) * c;      
    }                                                               
    if (val_x < 0)                                                      
        arct = A_180 - arct;                                              
    if (val_y < 0)                                                      
        arct = A_360 - arct;                                              
    
    dst[x * dsx + y * dsy] = (uchar)(int)floor(arct + 0.5f);
}
