#include <stdio.h>

#include "11__include_multitu_fnptr_table_callback_route_runtime.h"

static int callback_inc_add(int a, int b) {
    return a + b + 1;
}

static int callback_inc_mul(int a, int b) {
    return a * b + 1;
}

int main(void) {
    int total = 0;
    total += fnptr_inc_callback_route_pick(3)(2, callback_inc_add);
    total += fnptr_inc_callback_route_pick(8)(3, callback_inc_mul);
    if (total != 16) return 1;
    printf("%d\n", total);
    return 0;
}
