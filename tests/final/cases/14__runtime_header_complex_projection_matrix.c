#include <complex.h>
#include <stdio.h>

static long scaled(double value) {
    return (long)(value * 1000.0 + (value >= 0.0 ? 0.5 : -0.5));
}

int main(void) {
    double complex z = -3.0 + 4.0 * I;
    double complex mirrored = conj(z);
    long mag = scaled(cabs(z));
    long arg = scaled(carg(z));
    long real = scaled(creal(mirrored));
    long imag = scaled(cimag(mirrored));

    printf("complex-proj mag=%ld arg=%ld mirror=%ld/%ld\n", mag, arg, real, imag);
    return mag == 5000L && arg == 2214L && real == -3000L && imag == -4000L ? 0 : 1;
}
