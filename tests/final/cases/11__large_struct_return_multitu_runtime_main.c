#include <stdio.h>

typedef struct FinalLargeStats {
    unsigned long long a;
    unsigned long long b;
    unsigned long long c;
    unsigned long long d;
    unsigned long long e;
} FinalLargeStats;

FinalLargeStats final_large_struct_make(int seed, int *seen);

int main(void) {
    int seen = 0;
    FinalLargeStats stats = final_large_struct_make(10, &seen);
    if (seen != 15) return 1;
    if (stats.a != 10ULL) return 2;
    if (stats.e != 14ULL) return 3;
    printf("%llu %llu %d\n", stats.a, stats.e, seen);
    return 0;
}
