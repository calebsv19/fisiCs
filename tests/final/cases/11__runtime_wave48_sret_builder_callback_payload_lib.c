struct block48 {
    int row[4];
};

struct payload48 {
    struct block48 blocks[2];
    long stamp;
};

typedef int (*cell48_cb)(int value, int factor);
typedef struct payload48 (*builder48_fn)(struct block48 seed, cell48_cb cb, int bias);

static struct payload48 wave48_build_a(struct block48 seed, cell48_cb cb, int bias) {
    struct payload48 out;
    int i;
    out.stamp = bias;
    for (i = 0; i < 4; i++) {
        out.blocks[0].row[i] = cb(seed.row[i], bias + 1);
        out.blocks[1].row[i] = seed.row[3 - i] + bias;
        out.stamp += out.blocks[0].row[i] * (i + 3);
        out.stamp += out.blocks[1].row[i] * (i + 7);
    }
    return out;
}

static struct payload48 wave48_build_b(struct block48 seed, cell48_cb cb, int bias) {
    struct payload48 out;
    int i;
    out.stamp = bias;
    for (i = 0; i < 4; i++) {
        out.blocks[0].row[i] = cb(seed.row[3 - i], bias + i);
        out.blocks[1].row[i] = seed.row[i] + cb(i + 1, bias);
        out.stamp += out.blocks[0].row[i] * (i + 1);
        out.stamp += out.blocks[1].row[i] * (i + 5);
    }
    return out;
}

builder48_fn wave48_select_builder(int selector) {
    if ((selector & 1) != 0) {
        return wave48_build_b;
    }
    return wave48_build_a;
}
