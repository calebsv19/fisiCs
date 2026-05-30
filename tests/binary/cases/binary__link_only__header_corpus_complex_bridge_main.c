#include <complex.h>

typedef union {
    double complex z;
    double lane[2];
} Wave10ComplexSeedPack;

struct Wave10ComplexPair {
    double complex left;
    double complex right;
};

struct Wave10ComplexPair wave10_complex_pair_build(double complex seed);

int main(void) {
    Wave10ComplexSeedPack seed;
    struct Wave10ComplexPair pair;

    seed.lane[0] = 1.0;
    seed.lane[1] = 0.5;
    pair = wave10_complex_pair_build(seed.z);
    return (creal(pair.left) != 0.0 && cimag(pair.right) != 0.0) ? 0 : 1;
}
