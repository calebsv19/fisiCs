#include <stdio.h>

struct wave49_block {
    int row[4];
};

struct wave49_payload {
    struct wave49_block blocks[2];
    long stamp;
};

typedef int (*wave49_cell_cb)(struct wave49_block seed, int index, int bias);

struct wave49_payload wave49_sret_aggregate_arg_callback_chain(struct wave49_block seed, wave49_cell_cb cb, int bias);

static int wave49_cell_mix(struct wave49_block seed, int index, int bias) {
    return seed.row[index] * (bias + index) + seed.row[3 - index];
}

int main(void) {
    struct wave49_block seed = {{3, 5, 8, 13}};
    struct wave49_payload got = wave49_sret_aggregate_arg_callback_chain(seed, wave49_cell_mix, 4);
    printf("%d %d %d %ld\n", got.blocks[0].row[0], got.blocks[0].row[3], got.blocks[1].row[1], got.stamp);
    return 0;
}
