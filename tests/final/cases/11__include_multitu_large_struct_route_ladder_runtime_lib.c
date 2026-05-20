#include "11__include_multitu_large_struct_route_ladder_runtime.h"

RouteLadderStatsInc route_inc_ladder_make(int seed, LadderFnInc first, LadderFnInc second, int *seen) {
    RouteLadderStatsInc stats;
    int a = first(seed);
    int b = second(a);
    stats.base = (unsigned long long)seed;
    stats.hop0 = (unsigned long long)a;
    stats.hop1 = (unsigned long long)b;
    stats.hop2 = (unsigned long long)(a + b);
    stats.tail = (unsigned long long)(seed + a + b + 8);
    if (seen) {
        *seen = seed + a + b;
    }
    return stats;
}
