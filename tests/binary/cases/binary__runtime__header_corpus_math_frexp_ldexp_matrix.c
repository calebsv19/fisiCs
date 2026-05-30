#include <float.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    int exp = 0;
    double mant = frexp(10.0, &exp);
    double rebuilt = ldexp(mant, exp);
    long mant1000 = (long)(mant * 1000.0 + 0.5);
    long rebuilt1000 = (long)(rebuilt * 1000.0 + 0.5);

    if (mant1000 != 625L || exp != 4) {
        return 1;
    }
    if (fabs(rebuilt - 10.0) > DBL_EPSILON) {
        return 2;
    }
    printf(
        "mant1000=%ld exp=%d rebuilt1000=%ld\n",
        mant1000,
        exp,
        rebuilt1000);
    return 0;
}
