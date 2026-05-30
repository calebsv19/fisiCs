#include <complex.h>

static long double header_corpus_wave10_complex_long_double_mix(long double complex value) {
    long double real = creall(value);
    long double imag = cimagl(value);
    long double complex mirrored = conjl(value);

    return real + imag + creall(mirrored);
}

int main(void) {
    long double complex sample = 1.5L + 0.75Li;
    return header_corpus_wave10_complex_long_double_mix(sample) > 0.0L ? 0 : 1;
}
