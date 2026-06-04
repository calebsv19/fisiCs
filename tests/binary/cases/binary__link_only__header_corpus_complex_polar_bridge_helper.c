#include <complex.h>

struct Wave31ComplexPolar {
    double mag;
    double arg;
    double real;
    double imag;
};

struct Wave31ComplexPolar wave31_complex_polar_bridge(double complex z) {
    struct Wave31ComplexPolar out;
    double complex mirrored = conj(z);
    out.mag = cabs(z);
    out.arg = carg(z);
    out.real = creal(mirrored);
    out.imag = cimag(mirrored);
    return out;
}
