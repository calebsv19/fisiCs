typedef int (*FoldFn)(int, int);

struct StepCfg {
    int seed;
    int bias;
};

int abi_fold_steps(struct StepCfg cfg, const int* values, int count, FoldFn fn) {
    int acc = cfg.seed;
    for (int i = 0; i < count; ++i) {
        int adjusted = values[i] + cfg.bias + i;
        acc = fn(acc, adjusted);
    }
    return acc;
}
