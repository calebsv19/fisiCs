#include <math.h>
#include <stdio.h>

int main(void) {
    double finite = 4.0;
    double zero = 0.0;
    double inf = 1.0 / zero;
    double nanv = zero / zero;
    int finite_flags = isfinite(finite) + isnormal(finite) + (fpclassify(finite) == FP_NORMAL);
    int inf_flags = isinf(inf) + (!isfinite(inf)) + (fpclassify(inf) == FP_INFINITE);
    int nan_flags = isnan(nanv) + (!isfinite(nanv)) + (fpclassify(nanv) == FP_NAN);
    int zero_flags = isfinite(zero) + (!isnormal(zero)) + (fpclassify(zero) == FP_ZERO);
    int sign_flags = signbit(-0.0) ? 1 : 0;

    printf("math-class flags=%d/%d/%d/%d sign=%d\n",
           finite_flags,
           inf_flags,
           nan_flags,
           zero_flags,
           sign_flags);
    return finite_flags == 3 && inf_flags == 3 && nan_flags == 3 && zero_flags == 3 && sign_flags == 1 ? 0 : 1;
}
