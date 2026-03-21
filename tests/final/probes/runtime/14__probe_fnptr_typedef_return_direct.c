#include <stdio.h>

typedef int (*UnaryFn)(int);

static int inc(int x) {
    return x + 1;
}

static int dec(int x) {
    return x - 1;
}

static UnaryFn choose_direct(int pick_inc) {
    return pick_inc ? inc : dec;
}

int main(void) {
    UnaryFn f = choose_direct(1);
    UnaryFn g = choose_direct(0);
    int a = f(9);
    int b = g(9);
    int c = choose_direct(1)(3);

    printf("%d %d %d\n", a, b, c);
    return 0;
}
