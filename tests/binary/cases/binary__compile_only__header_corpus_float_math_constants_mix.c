#include <float.h>
#include <math.h>

static double header_corpus_wave9_constant_blend(double value) {
    double scaled = ldexp(value, 1);
    double epsilon = DBL_EPSILON;
    return isfinite(value) ? (scaled + epsilon) : scaled;
}

int main(void) {
    volatile double sample = 0.5;
    return header_corpus_wave9_constant_blend(sample) > 0.0 ? 0 : 1;
}
