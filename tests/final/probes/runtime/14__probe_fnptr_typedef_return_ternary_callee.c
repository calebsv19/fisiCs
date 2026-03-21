#include <stdio.h>

typedef int (*UnaryFn)(int);

static int inc(int x) {
    return x + 1;
}

static int dec(int x) {
    return x - 1;
}

static UnaryFn choose_pair(UnaryFn a, UnaryFn b, int take_a) {
    return take_a ? a : b;
}

int main(void) {
    int x = (1 ? choose_pair(inc, dec, 1) : choose_pair(dec, inc, 1))(10);
    int y = (0 ? choose_pair(inc, dec, 1) : choose_pair(dec, inc, 1))(10);
    int z = (1 ? choose_pair : choose_pair)(inc, dec, 0)(5);

    printf("%d %d %d\n", x, y, z);
    return 0;
}

