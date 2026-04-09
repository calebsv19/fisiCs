#include <stdio.h>

typedef unsigned int u32;

typedef u32 (*dispatch_fn)(u32, u32);

#define ROR(value, shift) (((value) >> ((shift) & 31u)) | ((value) << ((32u - ((shift) & 31u)) & 31u)))
#define LANE_STEP(state, seed, salt) ((ROR((state), ((seed) & 7u)) + ((seed) ^ (salt))) * 1664525u + 1013904223u)

static u32 dispatch_op_a(u32 state, u32 seed) {
    return LANE_STEP(state ^ seed, seed, 0xA5A5A5A5u);
}

static u32 dispatch_op_b(u32 state, u32 seed) {
    return LANE_STEP(state + 0x12345678u, seed, 0xC0FEBABEu);
}

static u32 dispatch_op_c(u32 state, u32 seed) {
    return LANE_STEP(state * 3u + 1u, seed, 0x0F1F2E3Du);
}

static u32 dispatch_op_d(u32 state, u32 seed) {
    return LANE_STEP(state ^ (seed << 5u), seed, 0x5DEECE66Du);
}

static const dispatch_fn phase_table[8] = {
    dispatch_op_a,
    dispatch_op_b,
    dispatch_op_c,
    dispatch_op_d,
    dispatch_op_b,
    dispatch_op_a,
    dispatch_op_d,
    dispatch_op_c,
};

static u32 step_phase(u32 state, u32 seed) {
    u32 lane = (seed >> 2u) & 7u;
    u32 result = phase_table[lane](state, seed);
    if ((result & 1u) != 0u) {
        result ^= LANE_STEP(result, result + seed, 0x9E3779B9u);
    }
    return result + (seed << 3u);
}

int main(void) {
    u32 state = 0x31415926u;
    u32 seed = 0xBEEFCAFEu;

    for (u32 iteration = 0u; iteration < 512u; ++iteration) {
        seed = seed * 1103515245u + 12345u + iteration;
        state = step_phase(state, seed);

        switch ((state >> 16u) & 7u) {
            case 0u:
            case 1u:
                state ^= seed;
                break;
            case 2u:
                state += (seed >> 3u);
                break;
            case 3u:
                state = LANE_STEP(state, seed, (u32)iteration);
                break;
            case 4u:
                state = (state * 3u) + (seed * 11u);
                break;
            case 5u:
                state ^= 0xA5A5u;
                break;
            default:
                state = LANE_STEP(state, seed, 0x55AA55AAu) ^ 0xDEADBEEFu;
                break;
        }

        if ((state & 3u) == 0u) {
            state = state + (seed >> 1u);
        }
    }

    printf("%u\n", state ^ seed);
    return 0;
}
