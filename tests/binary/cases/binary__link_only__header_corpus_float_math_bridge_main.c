#include <float.h>

double header_corpus_wave9_float_math_bridge(double value, int *exp_out);

int main(void) {
    int exp = 0;
    double rebuilt = header_corpus_wave9_float_math_bridge(6.5, &exp);
    return (rebuilt > 6.5 && exp > 0 && DBL_EPSILON > 0.0) ? 0 : 1;
}
