struct wave49_block {
    int row[4];
};

struct wave49_payload {
    struct wave49_block blocks[2];
    long stamp;
};

typedef int (*wave49_cell_cb)(struct wave49_block seed, int index, int bias);

struct wave49_payload wave49_sret_aggregate_arg_callback_chain(struct wave49_block seed, wave49_cell_cb cb, int bias) {
    struct wave49_payload out;
    long stamp = bias;
    int i;

    for (i = 0; i < 4; i++) {
        int mixed = cb(seed, i, bias);
        out.blocks[0].row[i] = mixed + i;
        out.blocks[1].row[i] = seed.row[3 - i] + mixed - bias;
        stamp += out.blocks[0].row[i] * (i + 2);
        stamp += out.blocks[1].row[i] * (i + 5);
    }

    out.stamp = stamp;
    return out;
}
