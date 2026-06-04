#include <complex.h>

typedef struct {
    double complex z;
    double real;
    double imag;
} wave31_complex_projection_row;

static wave31_complex_projection_row wave31_project(double complex z) {
    wave31_complex_projection_row row;
    double complex mirrored = conj(z);
    row.z = mirrored;
    row.real = creal(mirrored);
    row.imag = cimag(mirrored);
    return row;
}

int main(void) {
    wave31_complex_projection_row row = wave31_project(-2.0 + 3.0 * I);
    return row.real < 0.0 && row.imag < 0.0 ? 0 : 1;
}
