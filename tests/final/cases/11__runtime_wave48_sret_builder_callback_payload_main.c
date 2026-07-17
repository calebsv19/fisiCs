#include <stdio.h>

struct block48 {
    int row[4];
};

struct payload48 {
    struct block48 blocks[2];
    long stamp;
};

typedef int (*cell48_cb)(int value, int factor);
typedef struct payload48 (*builder48_fn)(struct block48 seed, cell48_cb cb, int bias);

builder48_fn wave48_select_builder(int selector);

static int wave48_cell_fold(int value, int factor) {
    return value * factor + factor;
}

int main(void) {
    struct block48 seed = {{2, 4, 6, 8}};
    builder48_fn builder = wave48_select_builder(3);
    struct payload48 got = builder(seed, wave48_cell_fold, 2);
    printf("%d %d %d %ld\n", got.blocks[0].row[0], got.blocks[0].row[3], got.blocks[1].row[2], got.stamp);
    return 0;
}
