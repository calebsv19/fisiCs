#include <stdio.h>

typedef int (*op_t)(int, int);

static int add2(int a, int b) {
    return a + b;
}

static int sub2(int a, int b) {
    return a - b;
}

static int apply(op_t op, int x, int y) {
    return op(x, y);
}

int main(void) {
    int total = 0;
    int i = 0;

    for (i = 0; i < 5; ++i) {
        op_t op = (i & 1) ? sub2 : add2;
        int part = (op == sub2) ? apply(op, i * 3, i) : apply(op, i, i * 2);
        total += part;
        total += (i < 3) ? 1 : 2;
    }

    printf("%d\n", total);
    return 0;
}
