#include <float.h>

int wave29_float_range_surface(void) {
    int total = 0;

    total += FLT_MIN < FLT_MAX;
    total += DBL_MIN < DBL_MAX;
    total += LDBL_MIN < LDBL_MAX;
    total += FLT_EPSILON > 0.0f;
    total += DBL_EPSILON > 0.0;
    total += LDBL_EPSILON > 0.0L;
    total += FLT_MAX_EXP > FLT_MIN_EXP;
    total += DBL_MAX_10_EXP > DBL_MIN_10_EXP;
    return total;
}

int main(void) {
    return wave29_float_range_surface() == 8 ? 0 : 1;
}
