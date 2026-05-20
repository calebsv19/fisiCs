#include <stdio.h>

#include "11__include_multitu_large_struct_return_accumulate_runtime.h"

static unsigned long long large_bridge_inc_checksum(LargeBridgeStatsInc stats) {
    return stats.base + stats.delta + stats.mix + stats.total + stats.tail;
}

int main(void) {
    int seen = 0;
    LargeBridgeStatsInc stats = large_bridge_inc_make(9, 4, &seen);
    unsigned long long checksum = large_bridge_inc_checksum(stats);
    if (seen != 21) return 1;
    if (stats.base != 9ULL) return 2;
    if (stats.tail != 27ULL) return 3;
    if (checksum != 102ULL) return 4;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
