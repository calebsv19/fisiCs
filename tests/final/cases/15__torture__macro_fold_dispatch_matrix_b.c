#include <stdio.h>

typedef unsigned int U;
typedef U (*Op)(U, U);

static U op_xor(U a, U b) {
    return (a ^ (b << 1u)) + 0x9e3779b9u;
}

static U op_mul(U a, U b) {
    return (a * (b | 1u)) + 0x85ebca6bu;
}

static U op_mix(U a, U b) {
    return (a << ((b & 7u) + 1u)) ^ (a + b);
}

static U op_add(U a, U b) {
    return a + b + 0x7f4a7c15u;
}

static Op k_ops[4] = { op_xor, op_mul, op_mix, op_add };

#define FOLD(a, b, m) ((a ^ (b + m)) * 1664525u + ((a >> 3u) ^ (b << 5u)))
#define ROT(v, s) ((v << ((s) & 31u)) | (v >> ((32u - ((s) & 31u)) & 31u)))

static U apply(U state, U idx, U seed) {
    Op fn = k_ops[idx & 3u];
    U folded = FOLD(state, seed ^ (seed << 1u), fn(state, idx) ^ seed);
    return ROT(folded, idx + (seed & 7u)) ^ (idx * 0x9e3779b9u) ^ (state + seed);
}

int main(void) {
    U state = 0xA5A5A5A5u;
    U checksum = 0u;

    for (U i = 0u; i < 128u; ++i) {
        U idx = (state ^ (i * 31u)) & 3u;
        U seed = (state * 1103515245u) + 12345u + i;
        state = apply(state, idx, seed);
        checksum ^= (state + (seed << 1u)) ^ (seed >> 1u);
        if ((i & 7u) == 0u) {
            checksum = (checksum * 16777619u) ^ (state ^ i);
        }
    }

    printf("%u\n", checksum ^ state);
    return 0;
}
