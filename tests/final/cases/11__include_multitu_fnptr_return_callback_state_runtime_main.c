#include <stdio.h>

#include "11__include_multitu_fnptr_return_callback_state_runtime.h"

static int run_inc_state_route(StateRouteFnInc fn, int *state, int value) {
    return fn(state, value);
}

int main(void) {
    int state = 4;
    int total = 0;
    total += run_inc_state_route(fnptr_inc_state_route_pick(1), &state, 2);
    total += run_inc_state_route(fnptr_inc_state_route_pick(6), &state, 5);
    if (state != 13) return 1;
    if (total != 20) return 2;
    printf("%d %d\n", total, state);
    return 0;
}
