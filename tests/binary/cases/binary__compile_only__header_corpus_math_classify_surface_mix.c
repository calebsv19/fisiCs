#include <math.h>
#include <stdbool.h>

static bool header_corpus_wave9_classify_surface(double value) {
    double ipart = 0.0;
    double frac = modf(value, &ipart);
    int zeroish = (fpclassify(frac) == FP_ZERO) ? 1 : 0;
    int signed_frac = signbit(frac) ? 1 : 0;
    int normal_value = isnormal(value) ? 1 : 0;

    return zeroish || signed_frac || normal_value;
}

int main(void) {
    return header_corpus_wave9_classify_surface(-8.0) ? 0 : 1;
}
