#include <complex.h>

typedef union {
    double complex z;
    double lane[2];
} header_corpus_wave10_complex_pack;

static double header_corpus_wave10_complex_parts(double complex value) {
    double real = creal(value);
    double imag = cimag(value);
    double complex flipped = conj(value);

    return real + imag + creal(flipped);
}

int main(void) {
    header_corpus_wave10_complex_pack sample;
    sample.lane[0] = 1.25;
    sample.lane[1] = 2.5;
    return header_corpus_wave10_complex_parts(sample.z) > 0.0 ? 0 : 1;
}
