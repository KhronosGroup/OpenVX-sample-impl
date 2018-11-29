
#ifdef __cplusplus
extern "C" {
#endif
    float mant_adjust(float product, float layer_in, float layer_w);
    float sip_ml_mult(float op1, float op2);
    float sip_ml_add(float op1, float op2);
    float half2float_out(unsigned short value);
    unsigned short int float2half_out(float value, int round_style);
#ifdef __cplusplus
}
#endif

