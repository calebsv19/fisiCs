#include <stdio.h>

typedef unsigned int U;
typedef U (*dispatch_fn)(U, U);

static U dispatch_inc(U state, U seed) {
    return (state + seed + 0x9e3779b9u) ^ (state >> 3u);
}

static U dispatch_scramble(U state, U seed) {
    U rotated = (state << ((seed & 31u) + 1u)) | (state >> (31u - (seed & 31u)));
    return (rotated ^ (seed * 2654435761u)) + 0x7f4a7c15u;
}

static U dispatch_mix(U state, U seed) {
    U a = (state ^ (seed << 5u)) + 0x12345678u;
    U b = (state >> 1u) ^ (seed >> 3u);
    return a + (b << 2u) + (b >> 2u);
}

static U dispatch_xor(U state, U seed) {
    return (state * 1664525u) ^ (seed + 0xdeadbeefu);
}

static dispatch_fn dispatch_row0[] = { dispatch_inc, dispatch_scramble, dispatch_mix };
static dispatch_fn dispatch_row1[] = { dispatch_mix, dispatch_xor, dispatch_inc };

static U route_dispatch(U state, U step) {
    U idx = (state ^ (step * 0x9e3779b9u)) & 3u;
    U lane = idx & 1u;
    U op = (state + step) & 2u;

    if (op == 0u) {
        return dispatch_row0[lane * 2u](state + 0x11111111u, step + 0x1u) ^ (state << 1u);
    }
    if (op == 2u) {
        return dispatch_row1[(lane ^ 1u)](state + step, step + 0x01010101u) + (state & 0xffffu);
    }

    U branch = (state >> (idx + 1u)) & 1u;
    return dispatch_row0[branch](state ^ step, step ^ 0x0f0f0f0fu);
}

int main(void) {
    U state = 0x12345678u;
    U state_mix = 0x89abcdefu;

    for (U i = 0u; i < 1024u; ++i) {
        U step = (i * 37u) + (state_mix >> 1u);
        U value = route_dispatch(state, step);
        state ^= value;

        if ((i & 15u) == 0u) {
            state_mix = dispatch_row1[i & 1u](state_mix, value + 0x9e3779b9u);
            state += state_mix;
        }
    }

    state ^= state_mix;
    printf("%u\n", state);
    return 0;
}
