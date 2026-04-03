#include <stdio.h>

typedef int (*op_fn)(int, int);

static int add(int a, int b) { return a + b; }
static int sub(int a, int b) { return a - b; }
static int mul(int a, int b) { return a * b; }

static int apply(op_fn fn, int a, int b) {
    return fn(a, b);
}

int main(void) {
    op_fn ops[3];
    int total = 0;

    ops[0] = add;
    ops[1] = sub;
    ops[2] = mul;

    total += apply(ops[0], 9, 4);
    total += apply(ops[1], 9, 4);
    total += apply(ops[2], 9, 4);

    printf("%d\n", total);
    return 0;
}
