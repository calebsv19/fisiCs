#include <stdio.h>

typedef struct FnptrBridgeStats {
    unsigned long long left;
    unsigned long long right;
    unsigned long long combined;
    unsigned long long marker;
    unsigned long long tail;
} FnptrBridgeStats;

typedef int (*BridgeFn)(int, int);

FnptrBridgeStats large_fnptr_bridge_make(int seed, BridgeFn fn, int *seen);

static int bridge_mix(int a, int b) {
    return a * 2 + b;
}

int main(void) {
    int seen = 0;
    FnptrBridgeStats stats = large_fnptr_bridge_make(6, bridge_mix, &seen);
    unsigned long long checksum = stats.left + stats.right + stats.combined + stats.marker + stats.tail;
    if (seen != 19) return 1;
    if (stats.tail != 31ULL) return 2;
    if (checksum != 111ULL) return 3;
    printf("%llu %llu %d\n", checksum, stats.tail, seen);
    return 0;
}
