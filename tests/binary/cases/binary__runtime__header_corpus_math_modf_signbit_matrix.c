#include <float.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    double ipart = 0.0;
    double frac = modf(-3.75, &ipart);
    long frac100 = (long)(frac * 100.0);
    int sign_frac = signbit(frac) ? 1 : 0;
    int sign_int = signbit(ipart) ? 1 : 0;
    int sign_abs = signbit(fabs(frac)) ? 1 : 0;

    if (frac100 != -75L || (long)ipart != -3L) {
        return 1;
    }
    if (!sign_frac || !sign_int || sign_abs) {
        return 2;
    }
    if (!(DBL_EPSILON > 0.0)) {
        return 3;
    }

    printf(
        "frac100=%ld int=%ld sign=%d/%d/%d\n",
        frac100,
        (long)ipart,
        sign_frac,
        sign_int,
        sign_abs);
    return 0;
}
