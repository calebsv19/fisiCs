#include <stdio.h>

typedef int (*CallbackRouteFn)(int, int (*)(int, int));

CallbackRouteFn fnptr_callback_route_pick(int seed);

static int callback_route_add(int a, int b) {
    return a + b;
}

static int callback_route_mul(int a, int b) {
    return a * b;
}

int main(void) {
    int total = 0;
    total += fnptr_callback_route_pick(2)(3, callback_route_add);
    total += fnptr_callback_route_pick(5)(4, callback_route_mul);
    if (total != 18) return 1;
    printf("%d\n", total);
    return 0;
}
