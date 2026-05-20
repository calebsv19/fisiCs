#include "11__include_multitu_fnptr_return_table_callback_runtime.h"

static int inc_route_forward(int value, BinaryCbInc cb) {
    return cb(value, 1);
}

static int inc_route_split(int value, BinaryCbInc cb) {
    return cb(value, 1) + cb(value, 2);
}

RouteCbFnInc fnptr_inc_route_callback_pick(int seed) {
    RouteCbFnInc table[2];
    table[0] = inc_route_forward;
    table[1] = inc_route_split;
    return table[seed & 1];
}
