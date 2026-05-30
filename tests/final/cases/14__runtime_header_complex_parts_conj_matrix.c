#include <complex.h>
#include <stdio.h>

typedef union {
    double complex z;
    double lane[2];
} header_corpus_wave10_complex_pack;

int main(void) {
    header_corpus_wave10_complex_pack a;
    header_corpus_wave10_complex_pack delta;
    double complex b;
    long real100;
    long imag100;
    long mix100;

    a.lane[0] = 1.25;
    a.lane[1] = 2.5;
    delta.lane[0] = 0.5;
    delta.lane[1] = -0.25;
    b = conj(a.z) + delta.z;
    real100 = (long)(creal(b) * 100.0 + 0.5);
    imag100 = (long)(cimag(b) * 100.0 - 0.5);
    mix100 = (long)((creal(a.z) + cimag(a.z)) * 100.0 + 0.5);

    if (real100 != 175L) {
        return 1;
    }
    if (imag100 != -275L) {
        return 2;
    }
    if (mix100 != 375L) {
        return 3;
    }

    printf("real100=%ld imag100=%ld mix100=%ld\n", real100, imag100, mix100);
    return 0;
}
