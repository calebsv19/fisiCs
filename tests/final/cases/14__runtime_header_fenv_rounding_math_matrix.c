#include <fenv.h>
#include <math.h>
#include <stdio.h>

static long scaled(double value) {
    return (long)(value * 1000.0 + (value >= 0.0 ? 0.5 : -0.5));
}

int main(void) {
    int original = fegetround();
    double down_value;
    double up_value;
    double near_value;
    int down_flags;
    int up_flags;
    int near_flags;

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }
    if (fesetround(FE_DOWNWARD) != 0) {
        return 2;
    }
    down_value = nearbyint(2.75);
    down_flags = fetestexcept(FE_INEXACT);

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    if (fesetround(FE_UPWARD) != 0) {
        return 4;
    }
    up_value = nearbyint(2.25);
    up_flags = fetestexcept(FE_INEXACT);

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    if (fesetround(FE_TONEAREST) != 0) {
        return 6;
    }
    near_value = rint(2.5);
    near_flags = fetestexcept(FE_INEXACT);

    if (fesetround(original) != 0) {
        return 7;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }

    printf("fenv-round-math down=%ld flag=%d up=%ld flag=%d near=%ld flag=%d\n",
           scaled(down_value),
           (down_flags & FE_INEXACT) ? 1 : 0,
           scaled(up_value),
           (up_flags & FE_INEXACT) ? 1 : 0,
           scaled(near_value),
           (near_flags & FE_INEXACT) ? 1 : 0);

    return scaled(down_value) == 2000L && down_flags == 0 &&
                   scaled(up_value) == 3000L && up_flags == 0 &&
                   scaled(near_value) == 2000L && (near_flags & FE_INEXACT) != 0
               ? 0
               : 1;
}
