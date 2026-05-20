#include "11__include_multitu_large_struct_return_accumulate_runtime.h"

LargeBridgeStatsInc large_bridge_inc_make(int seed, int offset, int *seen) {
    LargeBridgeStatsInc stats;
    stats.base = (unsigned long long)seed;
    stats.delta = (unsigned long long)offset;
    stats.mix = (unsigned long long)(seed * offset);
    stats.total = (unsigned long long)(seed + offset + 13);
    stats.tail = (unsigned long long)(seed + offset + 14);
    if (seen) {
        *seen = seed + offset + 8;
    }
    return stats;
}
