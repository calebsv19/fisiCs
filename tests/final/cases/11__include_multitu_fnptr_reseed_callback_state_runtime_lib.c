#include "11__include_multitu_fnptr_reseed_callback_state_runtime.h"

static int inc_state_forward(int *cursor, int value) {
    *cursor += value + 1;
    return *cursor + value;
}

static int inc_state_twist(int *cursor, int value) {
    *cursor += value;
    return *cursor + value * 2;
}

StatefulCbFnInc fnptr_inc_reseed_state_pick(int seed, int *cursor) {
    StatefulCbFnInc table[2];
    table[0] = inc_state_forward;
    table[1] = inc_state_twist;
    *cursor += seed & 1;
    return table[(*cursor + seed + 1) & 1];
}
