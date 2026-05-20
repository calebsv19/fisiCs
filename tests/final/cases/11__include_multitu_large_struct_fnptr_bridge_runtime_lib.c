#include "11__include_multitu_large_struct_fnptr_bridge_runtime.h"

FnptrBridgeStatsInc large_fnptr_inc_bridge_make(int seed, BridgeFnInc fn, int *seen) {
    FnptrBridgeStatsInc stats;
    int fn_value = fn(seed, 4);
    stats.left = (unsigned long long)seed;
    stats.right = (unsigned long long)fn_value;
    stats.combined = (unsigned long long)(seed + fn_value);
    stats.marker = (unsigned long long)(fn_value + 19);
    stats.tail = (unsigned long long)(seed + fn_value + 14);
    if (seen) {
        *seen = fn_value + 5;
    }
    return stats;
}
