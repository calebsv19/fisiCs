#include <stdio.h>

#include "11__include_multitu_large_struct_fnptr_bridge_runtime.h"

static int bridge_inc_mix(int a, int b) {
    return a + b * 3;
}

int main(void) {
    int seen = 0;
    FnptrBridgeStatsInc stats = large_fnptr_inc_bridge_make(5, bridge_inc_mix, &seen);
    unsigned long long checksum = stats.left + stats.right + stats.combined + stats.marker + stats.tail;
    if (seen != 22) return 1;
    if (stats.tail != 36ULL) return 2;
    if (checksum != 116ULL) return 3;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
