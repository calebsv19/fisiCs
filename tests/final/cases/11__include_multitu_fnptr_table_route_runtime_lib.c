#include "11__include_multitu_fnptr_table_route_runtime.h"

static int route_inc_add4(int value) {
    return value + 4;
}

static int route_inc_mul2(int value) {
    return value * 2;
}

RouteFnInc fnptr_table_inc_pick(int seed) {
    RouteFnInc table[2];
    table[0] = route_inc_add4;
    table[1] = route_inc_mul2;
    return table[seed & 1];
}
