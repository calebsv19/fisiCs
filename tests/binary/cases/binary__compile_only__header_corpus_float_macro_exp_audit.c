#include <float.h>

int main(void) {
    int min_exp = FLT_MIN_EXP + DBL_MIN_EXP;
    int min_10_exp = FLT_MIN_10_EXP + DBL_MIN_10_EXP;
    int max_exp = FLT_MAX_EXP + DBL_MAX_EXP;
    int max_10_exp = FLT_MAX_10_EXP + DBL_MAX_10_EXP;

    return min_exp + min_10_exp + max_exp + max_10_exp;
}
