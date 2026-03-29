#include <stdio.h>

typedef int (*FoldFn)(int, int);

struct StepCfg {
    int seed;
    int bias;
};

int abi_fold_steps(struct StepCfg cfg, const int* values, int count, FoldFn fn);

static int fold_mul_add(int acc, int x) {
    return acc * 2 + x;
}

static int fold_xor_bias(int acc, int x) {
    return (acc ^ x) + 1;
}

int main(void) {
    int values[4] = {3, 1, 4, 1};
    struct StepCfg cfg;
    cfg.seed = 5;
    cfg.bias = 2;
    int left = abi_fold_steps(cfg, values, 4, fold_mul_add);
    int right = abi_fold_steps(cfg, values, 4, fold_xor_bias);
    printf("%d %d\n", left, right);
    return 0;
}
