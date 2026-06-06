#include <fenv.h>
#include <math.h>
#include <stdio.h>

static long scaled(double value) {
    return (long)(value * 1000.0 + (value >= 0.0 ? 0.5 : -0.5));
}

int main(void) {
    double pos_zero = 0.0;
    double neg_zero = -0.0;
    double inf = INFINITY;
    double nanv = NAN;
    double rem = remainder(7.0, 4.0);
    double dim = fdim(3.0, 5.0);
    double dim2 = fdim(9.0, 4.0);
    double copied = copysign(2.5, neg_zero);
    int unordered = isunordered(nanv, 1.0) ? 1 : 0;
    int less = isless(rem, pos_zero) ? 1 : 0;
    int inf_flag = isinf(inf) ? 1 : 0;
    int finite_flag = isfinite(copied) ? 1 : 0;
    int sign_flag = signbit(copied) ? 1 : 0;
    long summary = scaled(rem) + scaled(dim) + scaled(dim2) + scaled(copied) +
                   unordered + less + inf_flag + finite_flag + sign_flag;

    (void)feclearexcept(FE_ALL_EXCEPT);

    printf("math-fenv-special rem=%ld dim=%ld dim2=%ld copy=%ld flags=%d/%d/%d/%d/%d summary=%ld\n",
           scaled(rem),
           scaled(dim),
           scaled(dim2),
           scaled(copied),
           unordered,
           less,
           inf_flag,
           finite_flag,
           sign_flag,
           summary);

    return scaled(rem) == -1000L && scaled(dim) == 0L && scaled(dim2) == 5000L &&
                   scaled(copied) == -2500L && unordered == 1 && less == 1 &&
                   inf_flag == 1 && finite_flag == 1 && sign_flag == 1 &&
                   summary == 1505L
               ? 0
               : 1;
}
