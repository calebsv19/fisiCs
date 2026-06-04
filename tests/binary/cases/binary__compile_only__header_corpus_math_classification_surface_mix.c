#include <math.h>

int wave30_math_classification_surface(double value) {
    int total = 0;

    total += isfinite(value) ? 1 : 0;
    total += isnormal(value) ? 1 : 0;
    total += isnan(value) ? 1 : 0;
    total += isinf(value) ? 1 : 0;
    total += fpclassify(value) == FP_NORMAL ? 1 : 0;
    total += signbit(value) ? 1 : 0;
    return total;
}

int main(void) {
    return 0;
}
