#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    double one = 1.0;
    double zero = 0.0;
    char* end_inf = 0;
    char* end_nan = 0;
    double pos_inf = strtod("inf", &end_inf);
    double nan_v = strtod("nan", &end_nan);
    int finite_one = isfinite(one) ? 1 : 0;
    int finite_zero = isfinite(zero) ? 1 : 0;
    int finite_inf = isfinite(pos_inf) ? 1 : 0;
    int finite_nan = isfinite(nan_v) ? 1 : 0;
    int normal_one = isnormal(one) ? 1 : 0;
    int normal_zero = isnormal(zero) ? 1 : 0;
    int normal_inf = isnormal(pos_inf) ? 1 : 0;
    int normal_nan = isnormal(nan_v) ? 1 : 0;

    if (!end_inf || !end_nan || end_inf[0] != '\0' || end_nan[0] != '\0') {
        return 2;
    }

    if (!(finite_one && finite_zero && !finite_inf && !finite_nan &&
          normal_one && !normal_zero && !normal_inf && !normal_nan)) {
        return 1;
    }

    printf("math_isfinite_isnormal_matrix_ok finite=%d/%d/%d/%d normal=%d/%d/%d/%d\n",
           finite_one, finite_zero, finite_inf, finite_nan,
           normal_one, normal_zero, normal_inf, normal_nan);
    return 0;
}
