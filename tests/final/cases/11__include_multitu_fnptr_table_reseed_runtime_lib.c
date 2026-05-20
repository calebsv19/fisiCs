#include "11__include_multitu_fnptr_table_reseed_runtime.h"

static int route_inc_reseed_add(int value, int *cursor) {
    *cursor += 2;
    return value + *cursor;
}

static int route_inc_reseed_mul(int value, int *cursor) {
    *cursor += 3;
    return value * 2 + *cursor;
}

ReseedRouteFnInc fnptr_table_inc_reseed_pick(int seed, int *cursor) {
    ReseedRouteFnInc table[2];
    table[0] = route_inc_reseed_add;
    table[1] = route_inc_reseed_mul;
    *cursor += seed & 1;
    return table[(seed + *cursor) & 1];
}
