#include "11__include_multitu_fnptr_table_callback_route_runtime.h"

static int inc_route_once(int value, BinaryCallbackInc cb) {
    return cb(value, 2);
}

static int inc_route_twice(int value, BinaryCallbackInc cb) {
    return cb(value, 1) + cb(value, 2);
}

CallbackRouteFnInc fnptr_inc_callback_route_pick(int seed) {
    CallbackRouteFnInc table[2];
    table[0] = inc_route_once;
    table[1] = inc_route_twice;
    return table[seed & 1];
}
