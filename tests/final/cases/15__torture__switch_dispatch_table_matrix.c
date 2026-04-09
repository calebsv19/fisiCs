#include <stdio.h>

typedef unsigned int U;
typedef U (*Handler)(U, U);

static U dispatch_add(U a, U b) {
    return a + b + 3u;
}

static U dispatch_sub(U a, U b) {
    return a - b;
}

static U dispatch_xor(U a, U b) {
    return a ^ (b + 0x3au);
}

static U dispatch_mix(U a, U b) {
    return (a << ((b & 7u) + 1u)) ^ (b >> 1u);
}

static Handler dispatch_table[4] = { dispatch_add, dispatch_sub, dispatch_xor, dispatch_mix };

static U spread_rot(U value, U shift) {
    return (value >> (shift & 31u)) | (value << ((31u - (shift & 31u)) & 31u));
}

static U route(U state, U step) {
    U next = (state + step) & 3u;
    state = spread_rot(state, next) ^ step;

    switch (next) {
        case 0u: {
            state = dispatch_table[0](state, next + 0x11111111u);
            break;
        }
        case 1u: {
            state = dispatch_table[1](state, next + 0x22222222u);
            break;
        }
        case 2u: {
            state = dispatch_table[2](state, next + 0x33333333u);
            break;
        }
        default: {
            state = dispatch_table[3](state, next + 0x44444444u);
            break;
        }
    }

    return state ^ (state << 3u) ^ (state >> 2u);
}

int main(void) {
    U state = 0x12345678u;
    U tally = 0xabcdef01u;

    for (U i = 0u; i < 512u; ++i) {
        state = route(state, (i * 2654435761u) ^ (tally >> 1u));
        tally += state;
        if ((state & 0x1fu) == 0u) {
            tally = (tally * 16777619u) + (state >> 1u);
        }
    }

    printf("%u\n", state ^ tally);
    return 0;
}
