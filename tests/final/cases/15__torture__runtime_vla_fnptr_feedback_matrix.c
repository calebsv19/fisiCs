#include <stdio.h>

typedef int (*ReducerFn)(int, int, int);

static int lane_add(int state, int cell, int bias) {
    return (state + cell + bias + 7) % 65521;
}

static int lane_mix(int state, int cell, int bias) {
    return (state * 5 + cell * 3 + bias * 11 + 13) % 65521;
}

static int lane_xor(int state, int cell, int bias) {
    unsigned us = (unsigned)(state & 0xFFFF);
    unsigned uc = (unsigned)(cell & 0xFFFF);
    unsigned ub = (unsigned)(bias & 0xFFFF);
    return (int)((us ^ (uc * 9u + ub * 7u + 17u)) % 65521u);
}

static int run_feedback(int rows, int cols, int seed, ReducerFn fn) {
    int total = seed;
    for (int r = 0; r < rows; ++r) {
        int vla_cols = cols + (r % 3);
        int line[vla_cols];
        for (int c = 0; c < vla_cols; ++c) {
            line[c] = (seed + r * 19 + c * 23 + total) % 1021;
        }
        for (int c = 0; c < vla_cols; ++c) {
            total = fn(total, line[c], (r + 1) * (c + 3));
        }
    }
    return total;
}

int main(void) {
    ReducerFn lanes[3] = {lane_add, lane_mix, lane_xor};
    int acc = 0;

    for (int i = 0; i < 8; ++i) {
        int rows = 3 + (i % 4);
        int cols = 4 + (i % 5);
        int seed = 97 + i * 31;
        int lane = i % 3;
        acc += run_feedback(rows, cols, seed, lanes[lane]) + lane * 101 - i * 17;
    }

    printf("%d\n", acc);
    return 0;
}
