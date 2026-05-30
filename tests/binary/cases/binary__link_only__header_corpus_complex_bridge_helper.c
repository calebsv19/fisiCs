#include <complex.h>

typedef union {
    double complex z;
    double lane[2];
} Wave10ComplexSeedPack;

struct Wave10ComplexPair {
    double complex left;
    double complex right;
};

struct Wave10ComplexPair wave10_complex_pair_build(double complex seed) {
    struct Wave10ComplexPair pair;
    Wave10ComplexSeedPack delta;

    delta.lane[0] = 0.25;
    delta.lane[1] = 0.75;
    pair.left = seed + conj(seed);
    pair.right = seed - delta.z;
    return pair;
}
