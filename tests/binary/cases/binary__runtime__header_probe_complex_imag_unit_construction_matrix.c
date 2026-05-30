#include <complex.h>
#include <stdio.h>

int main(void) {
    double complex a = 1.25 + 2.5 * I;
    double real100 = creal(a) * 100.0;
    double imag100 = cimag(a) * 100.0;

    if (real100 < 124.5 || real100 > 125.5) {
        return 1;
    }
    if (imag100 < 249.5 || imag100 > 250.5) {
        return 2;
    }

    printf("real100=%.0f imag100=%.0f\n", real100, imag100);
    return 0;
}
