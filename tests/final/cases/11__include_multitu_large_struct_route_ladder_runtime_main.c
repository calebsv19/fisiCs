#include <stdio.h>

#include "11__include_multitu_large_struct_route_ladder_runtime.h"

static int ladder_inc_add2(int value) {
    return value + 2;
}

static int ladder_inc_mul3(int value) {
    return value * 3;
}

int main(void) {
    int seen = 0;
    RouteLadderStatsInc stats = route_inc_ladder_make(3, ladder_inc_add2, ladder_inc_mul3, &seen);
    unsigned long long checksum = stats.base + stats.hop0 + stats.hop1 + stats.hop2 + stats.tail;
    if (seen != 23) return 1;
    if (stats.tail != 31ULL) return 2;
    if (checksum != 74ULL) return 3;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
