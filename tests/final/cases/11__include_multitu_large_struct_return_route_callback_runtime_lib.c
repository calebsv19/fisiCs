#include "11__include_multitu_large_struct_return_route_callback_runtime.h"

ReturnRouteStatsInc return_inc_route_stats_make(int seed, ReturnRouteFnInc pick0, ReturnRouteFnInc pick1, int *seen) {
    ReturnRouteStatsInc stats;
    int a = pick0(seed);
    int b = pick1(seed);
    stats.base = (unsigned long long)seed;
    stats.first = (unsigned long long)a;
    stats.second = (unsigned long long)b;
    stats.mix = (unsigned long long)(a + b);
    stats.tail = (unsigned long long)(seed + a + b + 4);
    if (seen) {
        *seen = seed + a + b;
    }
    return stats;
}
