#include "11__include_multitu_fnptr_callback_state_runtime.h"

int fnptr_callback_inc_state_fold(int *state, int count, StateCallbackInc cb) {
    int total = 0;
    int i;
    for (i = 0; i < count; ++i) {
        total += cb(state, i);
    }
    return total;
}
