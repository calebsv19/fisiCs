#include <stdio.h>

typedef int (*StepFn)(int, int);

static int step_add(int s, int b) {
    return s + b * 2 + 1;
}

static int step_xor(int s, int b) {
    return (s ^ (b * 11 + 3)) + (s & 5);
}

static int step_mix(int s, int b) {
    return s - b * 3 + 17;
}

struct Cell {
    StepFn fn;
    int bias;
};

static int run(struct Cell cells[3], int seed) {
    int state = seed;

    for (int i = 0; i < 10; ++i) {
        int idx = (state + i + cells[i % 3].bias) % 3;
        if (idx < 0) {
            idx += 3;
        }

        struct Cell *cell = &cells[idx];
        StepFn fn = (i & 1) ? cell->fn : cells[(idx + 1) % 3].fn;
        state = fn(state, cell->bias + i);

        if (state < 0) {
            state = -state + 5;
        }
        state %= 2000;
        cell->bias = (cell->bias * 3 + i + 2) % 17;
    }

    return state + cells[0].bias + cells[1].bias + cells[2].bias;
}

int main(void) {
    struct Cell cells[3] = {
        {step_add, 4},
        {step_xor, 6},
        {step_mix, 3},
    };

    printf("%d\n", run(cells, 9));
    return 0;
}
