#include <stdio.h>

typedef int (*Op)(int, int);

static int add(int a, int b) {
    return a + b;
}

static int sub(int a, int b) {
    return a - b;
}

static int mul(int a, int b) {
    return a * b;
}

int main(void) {
    Op table[3] = {add, sub, mul};
    int acc = 0;
    int lhs[3] = {7, 12, 3};
    int rhs[3] = {4, 5, 6};

    for (int i = 0; i < 3; ++i) {
        acc += table[i](lhs[i], rhs[i]);
    }

    printf("%d\n", acc);
    return 0;
}
