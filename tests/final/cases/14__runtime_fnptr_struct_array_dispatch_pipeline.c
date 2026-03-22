#include <stdio.h>

typedef int (*StepFn)(int, int);

typedef struct Dispatch {
    StepFn fn;
    int bias;
} Dispatch;

typedef Dispatch (*BuildFn)(int);

static int op_add(int a, int b) {
    return a + b;
}

static int op_sub(int a, int b) {
    return a - b;
}

static int op_xor(int a, int b) {
    return a ^ b;
}

static Dispatch make_add(int seed) {
    Dispatch d = {op_add, seed};
    return d;
}

static Dispatch make_sub(int seed) {
    Dispatch d = {op_sub, -seed};
    return d;
}

static Dispatch make_xor(int seed) {
    Dispatch d = {op_xor, seed * 2};
    return d;
}

static int eval_dispatch(Dispatch d, int x, int y) {
    return d.fn(x, y) + d.bias;
}

int main(void) {
    BuildFn builders[3] = {make_add, make_sub, make_xor};
    Dispatch cache[3];

    for (int i = 0; i < 3; ++i) {
        cache[i] = builders[i](i + 1);
    }

    int accum = 0;
    for (int i = 0; i < 6; ++i) {
        int lane = (i + 1) % 3;
        Dispatch from_builder = builders[lane](i + 2);
        Dispatch selected = (i & 1) ? from_builder : cache[lane];
        Dispatch piped = (i == 4) ? builders[0](3) : selected;
        accum += eval_dispatch(piped, i + 5, lane + 1);
    }

    int tail = eval_dispatch((0 ? builders[1] : builders[2])(4), 9, 3);
    printf("%d %d\n", accum, tail);
    return 0;
}
