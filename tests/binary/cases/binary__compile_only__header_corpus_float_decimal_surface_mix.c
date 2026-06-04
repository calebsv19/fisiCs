#include <float.h>

int wave29_float_decimal_surface(void) {
    int total = 0;

    total += FLT_RADIX >= 2;
    total += FLT_MANT_DIG > 0;
    total += DBL_MANT_DIG >= FLT_MANT_DIG;
    total += LDBL_MANT_DIG >= DBL_MANT_DIG;
    total += FLT_DIG > 0;
    total += DBL_DIG >= FLT_DIG;
    total += DECIMAL_DIG >= DBL_DIG;
    total += FLT_EVAL_METHOD >= -1;
    return total;
}

int main(void) {
    return wave29_float_decimal_surface() == 8 ? 0 : 1;
}
