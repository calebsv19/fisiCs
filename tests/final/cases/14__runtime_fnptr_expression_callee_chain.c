#include <stdio.h>

typedef int (*UnaryFn)(int);

static int mul2(int x) {
    return x * 2;
}

static int add3(int x) {
    return x + 3;
}

static UnaryFn bounce(UnaryFn fn) {
    return fn;
}

static int apply(UnaryFn fn, int value) {
    return fn(value);
}

int main(void) {
    UnaryFn table[2] = {mul2, add3};
    int a = bounce(table[0])(7);
    int b = (bounce(table[1]))(7);
    int c = apply((1 ? bounce(table[0]) : bounce(table[1])), 4);
    int d = (0 ? bounce(table[0]) : bounce(table[1]))(9);

    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
