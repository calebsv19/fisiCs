#include <float.h>

int main(void) {
    int radix = FLT_RADIX;
    int float_mant = FLT_MANT_DIG;
    int double_mant = DBL_MANT_DIG;
    int float_digits = FLT_DIG;
    int double_digits = DBL_DIG;

    return (radix > 0 && float_mant > 0 && double_mant > 0 &&
            float_digits > 0 && double_digits > 0)
               ? 0
               : 1;
}
