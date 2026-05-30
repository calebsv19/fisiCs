#include <float.h>
#include <math.h>

double header_corpus_wave9_float_math_bridge(double value, int *exp_out) {
    double mant = frexp(value, exp_out);
    return ldexp(mant, *exp_out) + DBL_EPSILON;
}
