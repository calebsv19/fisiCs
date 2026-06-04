#include <complex.h>

struct Wave31ComplexPolar {
    double mag;
    double arg;
    double real;
    double imag;
};

struct Wave31ComplexPolar wave31_complex_polar_bridge(double complex z);

int main(void) {
    struct Wave31ComplexPolar out = wave31_complex_polar_bridge(-3.0 + 4.0 * I);
    return out.mag > 0.0 && out.arg > 0.0 && out.real < 0.0 && out.imag < 0.0 ? 0 : 1;
}
