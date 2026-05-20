#include <stdio.h>

#include "11__include_multitu_fnptr_return_table_callback_runtime.h"

static int cb_inc_plus(int a, int b) {
    return a + b + 1;
}

static int cb_inc_weight(int a, int b) {
    return a * 2 + b + 1;
}

int main(void) {
    int total = 0;
    total += fnptr_inc_route_callback_pick(1)(2, cb_inc_plus);
    total += fnptr_inc_route_callback_pick(4)(3, cb_inc_weight);
    if (total != 17) return 1;
    printf("%d\n", total);
    return 0;
}
