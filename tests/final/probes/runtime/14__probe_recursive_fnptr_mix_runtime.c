#include <stdio.h>

typedef int (*StepFn)(int, int);

static int step_add(int base, int n) {
    return base + n * 2;
}

static int step_mix(int base, int n) {
    return base + (n * 3) - (n & 1);
}

static int walk(int n, StepFn step) {
    if (n <= 0) return 1;
    int prev = walk(n - 1, step);
    return step(prev, n);
}

int main(void) {
    StepFn table[2] = {step_add, step_mix};

    int acc = 0;
    for (int i = 0; i < 5; ++i) {
        StepFn s = (i & 1) ? table[1] : table[0];
        acc += walk(i + 3, s);
    }

    int tail = walk(6, (0 ? table[0] : table[1]));
    printf("%d %d\n", acc, tail);
    return 0;
}

