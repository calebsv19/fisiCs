#include <stdio.h>

typedef unsigned int u32;

typedef u32 (*dispatch_fn)(u32, u32);

typedef struct {
    dispatch_fn fn;
    u32 bonus;
} dispatch_cell;

static u32 dispatch_lane0(u32 state, u32 seed) {
    return (state + seed ^ 0x01234567u) + (seed >> 1u);
}

static u32 dispatch_lane1(u32 state, u32 seed) {
    return (state + seed * 3u) ^ 0x89ABCDEFu;
}

static u32 dispatch_lane2(u32 state, u32 seed) {
    return (state ^ (state >> 3u)) + (seed << 2u);
}

static u32 dispatch_lane3(u32 state, u32 seed) {
    return (state * 5u + 11u) ^ (seed + 0x2468ACE1u);
}

static dispatch_cell lanes[4] = {
    {dispatch_lane0, 0x11111111u},
    {dispatch_lane1, 0x22222222u},
    {dispatch_lane2, 0x33333333u},
    {dispatch_lane3, 0x44444444u},
};

static u32 run_dispatch(u32 state, u32 seed) {
    u32 thread = (seed >> 1u) & 3u;
    dispatch_cell cell = lanes[thread];
    u32 next = cell.fn(state ^ cell.bonus, seed);
    return next + cell.bonus;
}

int main(void) {
    u32 state = 0x0F0F0F0Fu;
    u32 seed = 0x87654321u;

    for (u32 i = 0u; i < 640u; ++i) {
        seed = (seed * 1664525u) + 1013904223u + i;
        state = run_dispatch(state, seed);

        switch ((state + seed) & 7u) {
            case 0u:
            case 1u:
                state ^= seed + 0x1u;
                break;
            case 2u:
                state += 7u;
                break;
            case 3u:
                state = (state >> 1u) | (state << 31u);
                break;
            case 4u:
            case 5u:
                state = (state * 5u) ^ 0xABCDEF01u;
                break;
            case 6u:
                state ^= (state >> 11u);
                break;
            default:
                state += 0xDEADC0DEu;
                break;
        }

        if ((state & 1u) != 0u) {
            state ^= (seed << 7u) ^ 0xA5A5A5A5u;
        }
    }

    printf("%u\n", state);
    return 0;
}
