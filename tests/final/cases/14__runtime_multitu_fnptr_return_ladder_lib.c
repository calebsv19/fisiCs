typedef int (*OpFn)(int, int);

struct OpCfg {
    int bias;
    int mode;
};

static int op_add(int acc, int x) {
    return acc + x;
}

static int op_mix(int acc, int x) {
    return (acc * 3) ^ (x + 7);
}

static int op_sub(int acc, int x) {
    return acc - x * 2;
}

OpFn abi_choose_op(struct OpCfg cfg) {
    int mode = cfg.mode % 3;
    if (mode == 0) {
        return op_add;
    }
    if (mode == 1) {
        return op_mix;
    }
    return op_sub;
}

int abi_apply_series(struct OpCfg cfg, const int* values, int count) {
    OpFn fn = abi_choose_op(cfg);
    int acc = cfg.bias;
    for (int i = 0; i < count; ++i) {
        acc = fn(acc, values[i] + i);
    }
    return acc;
}
