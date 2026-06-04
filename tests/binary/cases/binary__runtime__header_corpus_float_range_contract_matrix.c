#include <float.h>
#include <stdio.h>

int main(void) {
    int minmax = FLT_MIN < FLT_MAX && DBL_MIN < DBL_MAX && LDBL_MIN < LDBL_MAX;
    int exp_order = FLT_MIN_EXP < FLT_MAX_EXP && DBL_MIN_EXP < DBL_MAX_EXP &&
                    LDBL_MIN_EXP < LDBL_MAX_EXP;
    int decimal_order = FLT_MIN_10_EXP < FLT_MAX_10_EXP && DBL_MIN_10_EXP < DBL_MAX_10_EXP &&
                        LDBL_MIN_10_EXP < LDBL_MAX_10_EXP;
    int eps_positive = FLT_EPSILON > 0.0f && DBL_EPSILON > 0.0 && LDBL_EPSILON > 0.0L;
    int score = minmax + exp_order + decimal_order + eps_positive;

    printf("float-range score=%d exp=%d/%d/%d/%d dec=%d/%d/%d/%d\n",
           score,
           FLT_MIN_EXP,
           FLT_MAX_EXP,
           DBL_MIN_EXP,
           DBL_MAX_EXP,
           FLT_MIN_10_EXP,
           FLT_MAX_10_EXP,
           DBL_MIN_10_EXP,
           DBL_MAX_10_EXP);
    return score == 4 ? 0 : 1;
}
