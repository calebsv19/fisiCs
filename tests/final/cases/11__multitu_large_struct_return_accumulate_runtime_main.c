#include <stdio.h>

typedef struct LargeBridgeStats {
    unsigned long long base;
    unsigned long long delta;
    unsigned long long mix;
    unsigned long long total;
    unsigned long long tail;
} LargeBridgeStats;

LargeBridgeStats large_bridge_make(int seed, int offset, int *seen);

static unsigned long long large_bridge_checksum(LargeBridgeStats stats) {
    return stats.base + stats.delta + stats.mix + stats.total + stats.tail;
}

int main(void) {
    int seen = 0;
    LargeBridgeStats stats = large_bridge_make(7, 3, &seen);
    unsigned long long checksum = large_bridge_checksum(stats);
    if (seen != 17) return 1;
    if (stats.base != 7ULL) return 2;
    if (stats.tail != 24ULL) return 3;
    if (checksum != 76ULL) return 4;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
