#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    double zero = 0.0;
    double neg_zero = -0.0;
    char* end_nan = 0;
    char* end_inf = 0;
    double nan_v = strtod("nan", &end_nan);
    double pos_inf = strtod("inf", &end_inf);
    double neg_inf = -pos_inf;
    int ok_zero = (fpclassify(zero) == FP_ZERO) ? 1 : 0;
    int ok_neg_zero = (fpclassify(neg_zero) == FP_ZERO) ? 1 : 0;
    int ok_nan = (fpclassify(nan_v) == FP_NAN) ? 1 : 0;
    int ok_pos_inf = (fpclassify(pos_inf) == FP_INFINITE) ? 1 : 0;
    int ok_neg_inf = (fpclassify(neg_inf) == FP_INFINITE) ? 1 : 0;

    if (!end_nan || !end_inf || end_nan[0] != '\0' || end_inf[0] != '\0') {
        return 2;
    }

    if (!(ok_zero && ok_neg_zero && ok_nan && ok_pos_inf && ok_neg_inf)) {
        return 1;
    }

    printf("math_fpclassify_matrix_ok zero=%d neg_zero=%d nan=%d pos_inf=%d neg_inf=%d\n",
           ok_zero, ok_neg_zero, ok_nan, ok_pos_inf, ok_neg_inf);
    return 0;
}
