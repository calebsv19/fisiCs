typedef struct RouteLadderStats {
    unsigned long long base;
    unsigned long long hop0;
    unsigned long long hop1;
    unsigned long long hop2;
    unsigned long long tail;
} RouteLadderStats;

typedef int (*LadderFn)(int);

RouteLadderStats route_ladder_make(int seed, LadderFn first, LadderFn second, int *seen) {
    RouteLadderStats stats;
    int a = first(seed);
    int b = second(a);
    stats.base = (unsigned long long)seed;
    stats.hop0 = (unsigned long long)a;
    stats.hop1 = (unsigned long long)b;
    stats.hop2 = (unsigned long long)(a + b);
    stats.tail = (unsigned long long)(seed + a + b + 6);
    if (seen) {
        *seen = seed + a + b;
    }
    return stats;
}
