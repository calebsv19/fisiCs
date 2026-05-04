#include <stdio.h>

static int global_left = 5;
static int global_right = 9;

static int step(int x) {
    return x + 4;
}

static int twice(int x) {
    return x * 2;
}

struct Entry {
    const int *ptr;
    int (*fn)(int);
    const char *label;
};

static struct Entry table[3] = {
    [0] = {
        .ptr = &global_left,
        .fn = step,
        .label = "cpu",
    },
    [2] = {
        .label = "io",
        .fn = twice,
        .ptr = &global_right,
    },
};

int main(void) {
    printf(
        "%d %d %c %d %d\n",
        *table[0].ptr,
        table[0].fn(3),
        table[0].label[1],
        table[1].ptr == 0,
        table[2].fn(*table[2].ptr)
    );
    return 0;
}
