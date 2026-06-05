#include <math.h>
#include <stdio.h>

int main(void) {
    double nanv = NAN;
    double inf = INFINITY;
    double neg = -0.0;
    double copied = copysign(3.0, neg);
    double abs_nan = fabs(nanv);
    int nan_flag = isnan(nanv) ? 1 : 0;
    int inf_flag = isinf(inf) ? 1 : 0;
    int finite_flag = isfinite(copied) ? 1 : 0;
    int normal_flag = isnormal(copied) ? 1 : 0;
    int sign = signbit(copied) ? 1 : 0;
    int nan_stays_nan = isnan(abs_nan) ? 1 : 0;
    int unordered_flag = isunordered(nanv, 1.0) ? 1 : 0;
    int less_flag = isless(2.0, 3.0) ? 1 : 0;
    int greater_flag = isgreater(4.0, 3.0) ? 1 : 0;
    int le_flag = islessequal(3.0, 3.0) ? 1 : 0;
    int ge_flag = isgreaterequal(3.0, 3.0) ? 1 : 0;
    int score = nan_flag + inf_flag * 3 + finite_flag * 5 + normal_flag * 7 + sign * 11 +
                nan_stays_nan * 13 + unordered_flag * 17 + less_flag * 19 +
                greater_flag * 23 + le_flag * 29 + ge_flag * 31;

    printf("math-special nan=%d inf=%d finite=%d normal=%d sign=%d absnan=%d unordered=%d less=%d greater=%d le=%d ge=%d score=%d\n",
           nan_flag,
           inf_flag,
           finite_flag,
           normal_flag,
           sign,
           nan_stays_nan,
           unordered_flag,
           less_flag,
           greater_flag,
           le_flag,
           ge_flag,
           score);

    return nan_flag == 1 && inf_flag == 1 && finite_flag == 1 && normal_flag == 1 &&
                   sign == 1 && nan_stays_nan == 1 && unordered_flag == 1 &&
                   less_flag == 1 && greater_flag == 1 && le_flag == 1 &&
                   ge_flag == 1 && score == 159
               ? 0
               : 1;
}
