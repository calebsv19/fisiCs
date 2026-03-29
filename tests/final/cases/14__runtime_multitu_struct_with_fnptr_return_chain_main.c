#include <stdio.h>

typedef int (*CalcFn)(int, int);

struct CalcBox {
    int bias;
    CalcFn fn;
};

struct CalcBox abi_calc_pick(int mode, int bias);
struct CalcBox abi_calc_chain(struct CalcBox a, struct CalcBox b, int mode);
int abi_calc_apply(struct CalcBox box, int x, int y);

int main(void) {
    struct CalcBox a = abi_calc_pick(0, 3);
    struct CalcBox b = abi_calc_pick(1, -2);
    struct CalcBox c = abi_calc_chain(a, b, 2);
    int r0 = abi_calc_apply(a, 5, 7);
    int r1 = abi_calc_apply(b, 5, 7);
    int r2 = abi_calc_apply(c, 5, 7);
    printf("%d %d %d\n", r0, r1, r2);
    return 0;
}
