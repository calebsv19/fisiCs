#include "11__include_multitu_fnptr_return_callback_state_runtime.h"

static int state_inc_route_add(StateRouteFnInc fn, int *state, int value) {
    return fn(state, value);
}

static int state_inc_add(int *state, int value) {
    *state += value + 1;
    return *state;
}

static int state_inc_mix(int *state, int value) {
    *state += value;
    return value + *state + 1;
}

StateRouteFnInc fnptr_inc_state_route_pick(int seed) {
    StateRouteFnInc table[2];
    table[0] = state_inc_add;
    table[1] = state_inc_mix;
    return state_inc_route_add(table[(seed + 1) & 1], &seed, 0) ? table[seed & 1] : table[0];
}
