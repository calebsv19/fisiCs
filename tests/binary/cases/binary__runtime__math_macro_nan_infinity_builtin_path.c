#include <math.h>
#include <stdio.h>

int main(void) {
    double nan_v = NAN;
    double pos_inf = INFINITY;
    double neg_inf = -INFINITY;
    int ok_nan = (fpclassify(nan_v) == FP_NAN) ? 1 : 0;
    int ok_pos_inf = (fpclassify(pos_inf) == FP_INFINITE) ? 1 : 0;
    int ok_neg_inf = (fpclassify(neg_inf) == FP_INFINITE) ? 1 : 0;
    int nan_eq = (nan_v == nan_v) ? 1 : 0;

    if (!(ok_nan && ok_pos_inf && ok_neg_inf && !nan_eq)) {
        return 1;
    }

    printf("math_macro_nan_infinity_builtin_path_ok nan=%d pos_inf=%d neg_inf=%d\n",
           ok_nan, ok_pos_inf, ok_neg_inf);
    return 0;
}
