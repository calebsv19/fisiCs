#include <stdio.h>

typedef int (*StepFn)(int, int);

int abi_drive_steps(StepFn fn, int seed, const int* values, int count);

static int step_add(int acc, int x) {
    return acc + x * 2;
}

static int step_mix(int acc, int x) {
    return acc ^ (x + 3);
}

int main(void) {
    int values[5] = {1, 2, 3, 4, 5};
    int a = abi_drive_steps(step_add, 7, values, 5);
    int b = abi_drive_steps(step_mix, 9, values, 5);
    printf("%d %d\n", a, b);
    return 0;
}

