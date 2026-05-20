#include <stdio.h>

#include "11__include_multitu_large_struct_return_route_callback_runtime.h"

static int route_inc_add3(int value) {
    return value + 3;
}

static int route_inc_mul2(int value) {
    return value * 2;
}

int main(void) {
    int seen = 0;
    ReturnRouteStatsInc stats = return_inc_route_stats_make(4, route_inc_add3, route_inc_mul2, &seen);
    unsigned long long checksum = stats.base + stats.first + stats.second + stats.mix + stats.tail;
    if (seen != 19) return 1;
    if (stats.tail != 23ULL) return 2;
    if (checksum != 57ULL) return 3;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
