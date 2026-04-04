#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char* end_nan = 0;
    char* end_inf = 0;
    double nan_v = strtod("nan", &end_nan);
    double pos_inf = strtod("inf", &end_inf);
    double neg_inf = -pos_inf;
    int gt_inf = (pos_inf > 1.0) ? 1 : 0;
    int lt_ninf = (neg_inf < -1.0) ? 1 : 0;
    int nan_eq = (nan_v == nan_v) ? 1 : 0;
    int nan_lt = (nan_v < 0.0) ? 1 : 0;
    int nan_gt = (nan_v > 0.0) ? 1 : 0;

    if (!end_nan || !end_inf || end_nan[0] != '\0' || end_inf[0] != '\0') {
        return 2;
    }

    if (!(gt_inf && lt_ninf && !nan_eq && !nan_lt && !nan_gt)) {
        return 1;
    }

    printf("math_nan_inf_ordering_ok gt_inf=%d lt_ninf=%d nan_eq=%d nan_lt=%d nan_gt=%d\n",
           gt_inf, lt_ninf, nan_eq, nan_lt, nan_gt);
    return 0;
}
