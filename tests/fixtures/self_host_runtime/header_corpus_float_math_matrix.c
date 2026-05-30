#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static bool header_corpus_wave9_scale_and_split(
    double value,
    long *mant1000_out,
    int *exp_out,
    long *frac100_out,
    long *ipart_out,
    int *sign_frac_out,
    int *sign_abs_out
) {
    double ipart = 0.0;
    double frac = 0.0;
    double mant = 0.0;
    double rebuilt = 0.0;
    int exp = 0;

    if (!mant1000_out || !exp_out || !frac100_out || !ipart_out ||
        !sign_frac_out || !sign_abs_out) {
        return false;
    }

    mant = frexp(value, &exp);
    rebuilt = ldexp(mant, exp);
    frac = modf(-3.75, &ipart);

    if (fabs(rebuilt - value) > DBL_EPSILON) {
        return false;
    }

    *mant1000_out = (long)(mant * 1000.0 + 0.5);
    *exp_out = exp;
    *frac100_out = (long)(frac * 100.0);
    *ipart_out = (long)ipart;
    *sign_frac_out = signbit(frac) ? 1 : 0;
    *sign_abs_out = signbit(fabs(frac)) ? 1 : 0;
    return true;
}

int main(void) {
    long mant1000 = 0;
    long frac100 = 0;
    long ipart = 0;
    int exp = 0;
    int sign_frac = 0;
    int sign_abs = 0;
    bool ok = false;

    ok = header_corpus_wave9_scale_and_split(
        10.0,
        &mant1000,
        &exp,
        &frac100,
        &ipart,
        &sign_frac,
        &sign_abs);

    if (!ok) {
        return 1;
    }
    if (mant1000 != 625L || exp != 4) {
        return 2;
    }
    if (frac100 != -75L || ipart != -3L) {
        return 3;
    }
    if (!sign_frac || sign_abs) {
        return 4;
    }

    printf(
        "mant1000=%ld exp=%d frac100=%ld ipart=%ld sign=%d/%d\n",
        mant1000,
        exp,
        frac100,
        ipart,
        sign_frac,
        sign_abs);
    return 0;
}
