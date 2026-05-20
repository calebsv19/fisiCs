typedef struct ReturnRouteStats {
    unsigned long long base;
    unsigned long long first;
    unsigned long long second;
    unsigned long long mix;
    unsigned long long tail;
} ReturnRouteStats;

typedef int (*ReturnRouteFn)(int);

ReturnRouteStats return_route_stats_make(int seed, ReturnRouteFn pick0, ReturnRouteFn pick1, int *seen) {
    ReturnRouteStats stats;
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
