#include <stdio.h>

typedef unsigned int U;

#define BIND_STEP(idx) ((idx) * 1664525u + 1013904223u)
#define CHAIN_STEP(seed, shift) ((seed >> (shift)) | (seed << ((32u - (shift)) & 31u)))
#define DISPATCH_FN(idx) action##idx

static U action0(U x, U y) {
    return (x ^ y) + 0x13572468u;
}

static U action1(U x, U y) {
    return (x + y) ^ CHAIN_STEP(y, x & 31u);
}

static U action2(U x, U y) {
    return (x * 3u) + (y * 5u) + CHAIN_STEP(x, 11u);
}

static U action3(U x, U y) {
    return CHAIN_STEP(x ^ y, 7u) + CHAIN_STEP(y, 3u);
}

typedef U (*op_fn)(U, U);

static op_fn macro_matrix[2][2] = {
    { DISPATCH_FN(0), DISPATCH_FN(2) },
    { DISPATCH_FN(1), DISPATCH_FN(3) },
};

static U fold_step(U state, U seed) {
    U row = (seed >> 1u) & 1u;
    U col = seed & 1u;
    U next = macro_matrix[row][col](state + seed, seed ^ BIND_STEP(seed));
    next ^= CHAIN_STEP(next + seed, (next & 31u));
    return next;
}

int main(void) {
    U state = 0x0f0f0f0fu;
    U seed = 0x13579bdfu;

    for (U i = 0u; i < 768u; ++i) {
        seed = CHAIN_STEP(seed + i + state, (i & 31u));
        state ^= fold_step(state, seed);

        if ((i & 7u) == 0u) {
            seed = fold_step(seed, state ^ (i * 7u));
        }
        if ((state & 3u) == 0u) {
            state += (seed >> 16u);
        }
    }

    state ^= seed;
    printf("%u\n", state);
    return 0;
}
