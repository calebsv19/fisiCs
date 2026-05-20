#include "11__include_multitu_fnptr_callback_accumulate_runtime.h"

int fnptr_callback_inc_accumulate(int seed, int count, BridgeCallbackInc cb) {
    int total = 0;
    int i;
    for (i = 0; i < count; ++i) {
        total += cb(seed, i);
    }
    return total;
}
