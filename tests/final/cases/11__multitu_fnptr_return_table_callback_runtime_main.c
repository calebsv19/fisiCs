#include <stdio.h>

typedef int (*BinaryCb)(int, int);
typedef int (*RouteCbFn)(int, BinaryCb);

RouteCbFn fnptr_route_callback_pick(int seed);

static int cb_plus(int a, int b) {
    return a + b;
}

static int cb_weight(int a, int b) {
    return a * 2 + b;
}

int main(void) {
    int total = 0;
    total += fnptr_route_callback_pick(2)(3, cb_plus);
    total += fnptr_route_callback_pick(7)(4, cb_weight);
    if (total != 24) return 1;
    printf("%d\n", total);
    return 0;
}
