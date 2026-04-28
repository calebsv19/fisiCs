typedef struct FinalLargeStats {
    unsigned long long a;
    unsigned long long b;
    unsigned long long c;
    unsigned long long d;
    unsigned long long e;
} FinalLargeStats;

FinalLargeStats final_large_struct_make(int seed, int *seen) {
    FinalLargeStats stats;
    stats.a = (unsigned long long)seed;
    stats.b = (unsigned long long)(seed + 1);
    stats.c = (unsigned long long)(seed + 2);
    stats.d = (unsigned long long)(seed + 3);
    stats.e = (unsigned long long)(seed + 4);
    if (seen) {
        *seen = seed + 5;
    }
    return stats;
}
