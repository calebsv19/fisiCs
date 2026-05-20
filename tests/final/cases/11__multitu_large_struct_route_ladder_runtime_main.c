#include <stdio.h>

typedef struct RouteLadderStats {
    unsigned long long base;
    unsigned long long hop0;
    unsigned long long hop1;
    unsigned long long hop2;
    unsigned long long tail;
} RouteLadderStats;

typedef int (*LadderFn)(int);

RouteLadderStats route_ladder_make(int seed, LadderFn first, LadderFn second, int *seen);

static int ladder_add3(int value) {
    return value + 3;
}

static int ladder_mul2(int value) {
    return value * 2;
}

int main(void) {
    int seen = 0;
    RouteLadderStats stats = route_ladder_make(4, ladder_add3, ladder_mul2, &seen);
    unsigned long long checksum = stats.base + stats.hop0 + stats.hop1 + stats.hop2 + stats.tail;
    if (seen != 25) return 1;
    if (stats.tail != 31ULL) return 2;
    if (checksum != 77ULL) return 3;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
