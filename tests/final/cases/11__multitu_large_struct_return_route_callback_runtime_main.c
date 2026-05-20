#include <stdio.h>

typedef struct ReturnRouteStats {
    unsigned long long base;
    unsigned long long first;
    unsigned long long second;
    unsigned long long mix;
    unsigned long long tail;
} ReturnRouteStats;

typedef int (*ReturnRouteFn)(int);

ReturnRouteStats return_route_stats_make(int seed, ReturnRouteFn pick0, ReturnRouteFn pick1, int *seen);

static int route_add4(int value) {
    return value + 4;
}

static int route_mul3(int value) {
    return value * 3;
}

int main(void) {
    int seen = 0;
    ReturnRouteStats stats = return_route_stats_make(3, route_add4, route_mul3, &seen);
    unsigned long long checksum = stats.base + stats.first + stats.second + stats.mix + stats.tail;
    if (seen != 19) return 1;
    if (stats.tail != 23ULL) return 2;
    if (checksum != 58ULL) return 3;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
