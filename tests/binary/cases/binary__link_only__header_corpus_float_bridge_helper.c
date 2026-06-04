#include <float.h>

int wave29_float_bridge_score(void) {
    int total = 0;

    total += FLT_RADIX >= 2;
    total += FLT_ROUNDS >= -1;
    total += DBL_MANT_DIG >= FLT_MANT_DIG;
    total += LDBL_MANT_DIG >= DBL_MANT_DIG;
    total += FLT_MIN_EXP < FLT_MAX_EXP;
    total += DBL_MIN_EXP < DBL_MAX_EXP;
    total += LDBL_MIN_EXP < LDBL_MAX_EXP;
    return total;
}
