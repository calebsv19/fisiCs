typedef struct LargeBridgeStats {
    unsigned long long base;
    unsigned long long delta;
    unsigned long long mix;
    unsigned long long total;
    unsigned long long tail;
} LargeBridgeStats;

LargeBridgeStats large_bridge_make(int seed, int offset, int *seen) {
    LargeBridgeStats stats;
    stats.base = (unsigned long long)seed;
    stats.delta = (unsigned long long)offset;
    stats.mix = (unsigned long long)(seed * offset);
    stats.total = (unsigned long long)(seed + offset + 11);
    stats.tail = (unsigned long long)(seed + offset + 14);
    if (seen) {
        *seen = seed + offset + 7;
    }
    return stats;
}
