#include <stdio.h>

#include "11__include_multitu_fnptr_callback_state_runtime.h"

static int callback_inc_state_mix(int *state, int step) {
    *state += step + 1;
    return *state + step * 2;
}

int main(void) {
    int state = 4;
    int total = fnptr_callback_inc_state_fold(&state, 4, callback_inc_state_mix);
    if (state != 14) return 1;
    if (total != 48) return 2;
    printf("%d %d\n", total, state);
    return 0;
}
