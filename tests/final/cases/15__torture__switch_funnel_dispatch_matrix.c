#include <stdio.h>

typedef unsigned int U;
typedef U (*Op)(U, U);

static U op_add(U a, U b) {
    return a + b;
}

static U op_mul(U a, U b) {
    return a * b;
}

static U op_xor(U a, U b) {
    return a ^ b;
}

static U op_mix(U a, U b) {
    return ((a ^ 0x9e3779b9u) * 1664525u) + b;
}

static Op g_ops[4] = { op_add, op_mul, op_xor, op_mix };

static U run_dispatch(U state, U acc, U step) {
    for (unsigned round = 0u; round < 6u; ++round) {
        U idx = (state + round + (acc >> 16u)) & 3u;
        U base = (step + round + acc) ^ (acc << (idx + 1u));
        acc = g_ops[idx](acc ^ base, base + 0x13579u);

        switch (idx) {
            case 0u:
                acc += (acc >> 5u) + step;
                break;
            case 1u:
                acc ^= (acc << 3u) + (step << 1u);
                break;
            case 2u:
                acc = (acc >> 2u) + (base << 2u);
                break;
            default:
                acc ^= (acc >> 1u) + (acc << 4u);
                break;
        }

        state = (state + 1u) & 3u;
        acc ^= (state * 2654435761u);
    }

    return acc;
}

int main(void) {
    U seed = 0x9e3779b9u;
    U acc = 0x3f3fu;

    for (U step = 0u; step < 128u; ++step) {
        seed = (seed * 1103515245u) + 12345u + acc;
        acc = run_dispatch((seed + step) & 3u, acc, seed + step);

        if ((step & 31u) == 0u) {
            printf("%08x\n", (unsigned)(acc ^ seed));
        }
    }

    printf("%u\n", acc ^ seed);
    return 0;
}
