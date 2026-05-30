#include <complex.h>
#include <stdio.h>

int main(void) {
    long double complex a = 1.5L + 2.0Li;
    long double complex b = a * (0.5L - 1.0Li);
    long double complex c = conjl(a) + b;
    long real100 = (long)(creall(c) * 100.0L + 0.5L);
    long imag100 = (long)(cimagl(c) * 100.0L + (cimagl(c) >= 0.0L ? 0.5L : -0.5L));

    if (real100 != 425L) {
        return 1;
    }
    if (imag100 != -250L) {
        return 2;
    }

    printf("real100=%ld imag100=%ld\n", real100, imag100);
    return 0;
}
