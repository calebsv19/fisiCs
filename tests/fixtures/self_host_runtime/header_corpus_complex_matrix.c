#include <complex.h>

typedef union {
    double complex z;
    double lane[2];
} header_corpus_wave10_complex_pack;

static double complex header_corpus_wave10_make_double(double real, double imag) {
    header_corpus_wave10_complex_pack pack;
    pack.lane[0] = real;
    pack.lane[1] = imag;
    return pack.z;
}

static long double complex header_corpus_wave10_make_long_double(long double real, long double imag) {
    return real + imag * 1.0Li;
}

static double header_corpus_wave10_complex_summary(void) {
    double complex a = header_corpus_wave10_make_double(1.25, 2.5);
    double complex b = conj(a) + header_corpus_wave10_make_double(0.5, -0.25);
    return creal(b) + cimag(b) + creal(a) + cimag(a);
}

static long double header_corpus_wave10_complex_long_double_summary(void) {
    long double complex a = header_corpus_wave10_make_long_double(1.5L, 2.0L);
    long double complex b = a * header_corpus_wave10_make_long_double(0.5L, -1.0L);
    long double complex c = conjl(a) + b;
    return creall(c) + cimagl(c);
}

int main(void) {
    double summary = header_corpus_wave10_complex_summary();
    long double long_summary = header_corpus_wave10_complex_long_double_summary();
    return (summary != 0.0 && long_summary != 0.0L) ? 0 : 1;
}
