#include <tgmath.h>

double header_corpus_wave14_tgmath_bridge_result(double left, double right) {
    return sqrt(left) + sqrt(right) + fmax(left, right) + fmin(left, right) + pow(1.0, 3.0);
}
