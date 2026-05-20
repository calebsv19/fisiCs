typedef struct FnptrBridgeStats {
    unsigned long long left;
    unsigned long long right;
    unsigned long long combined;
    unsigned long long marker;
    unsigned long long tail;
} FnptrBridgeStats;

typedef int (*BridgeFn)(int, int);

FnptrBridgeStats large_fnptr_bridge_make(int seed, BridgeFn fn, int *seen) {
    FnptrBridgeStats stats;
    int fn_value = fn(seed, 5);
    stats.left = (unsigned long long)seed;
    stats.right = (unsigned long long)fn_value;
    stats.combined = (unsigned long long)(seed + fn_value);
    stats.marker = (unsigned long long)(fn_value + 17);
    stats.tail = (unsigned long long)(seed + fn_value + 8);
    if (seen) {
        *seen = fn_value + 2;
    }
    return stats;
}
