#include <stdio.h>

typedef int (*op2_fn)(int, int);

op2_fn abi_get_op(int id);
int abi_apply(op2_fn fn, int a, int b);

int main(void) {
    int value = 0;
    value += abi_apply(abi_get_op(2), 6, 7);
    value += abi_apply(abi_get_op(1), 9, 4);
    printf("%d\n", value);
    return 0;
}
