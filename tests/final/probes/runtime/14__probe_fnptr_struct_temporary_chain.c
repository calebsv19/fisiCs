#include <stdio.h>

typedef int (*BinFn)(int, int);

typedef struct Dispatch {
    BinFn fn;
    int bias;
} Dispatch;

typedef Dispatch (*BuilderFn)(int);

static int add(int a, int b) {
    return a + b;
}

static int sub(int a, int b) {
    return a - b;
}

static Dispatch build_add(int bias) {
    Dispatch d = {add, bias};
    return d;
}

static Dispatch build_sub(int bias) {
    Dispatch d = {sub, -bias};
    return d;
}

static int apply(Dispatch d, int x, int y) {
    return d.fn(x, y) + d.bias;
}

int main(void) {
    BuilderFn table[2] = {build_add, build_sub};

    Dispatch left = (0 ? table[0] : table[1])(3);
    int a = apply(left, 20, 7);

    int b = apply((1 ? table[0] : table[1])(2), 4, 6);

    int c = ((0 ? table[0] : table[1])(9)).fn(10, 3);

    printf("%d %d %d\n", a, b, c);
    return 0;
}
