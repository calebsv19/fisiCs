#include <stdio.h>

typedef int (*OpFn)(int, int);

struct OpCfg {
    int bias;
    int mode;
};

OpFn abi_choose_op(struct OpCfg cfg);
int abi_apply_series(struct OpCfg cfg, const int* values, int count);

int main(void) {
    int values[6] = {3, 1, 4, 1, 5, 9};
    struct OpCfg a;
    struct OpCfg b;
    a.bias = 2;
    a.mode = 1;
    b.bias = -1;
    b.mode = 2;

    OpFn f = abi_choose_op(a);
    int direct = f(10, 6);
    int s1 = abi_apply_series(a, values, 6);
    int s2 = abi_apply_series(b, values, 6);
    printf("%d %d %d\n", direct, s1, s2);
    return 0;
}
