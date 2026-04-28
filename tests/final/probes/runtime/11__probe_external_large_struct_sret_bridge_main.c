#include <stdio.h>

typedef struct ProbeStats {
    unsigned long long a;
    unsigned long long b;
    unsigned long long c;
    unsigned long long d;
    unsigned long long e;
} ProbeStats;

ProbeStats probe_external_large_struct_sret_bridge(const char *tag, int *seen);

int main(void) {
    int seen = 0;
    ProbeStats stats = probe_external_large_struct_sret_bridge("ok", &seen);
    printf("seen=%d tail=%llu\n", seen, stats.e);
    if (seen != 77) return 1;
    if (stats.a != 11ULL) return 2;
    if (stats.e != 55ULL) return 3;
    return 0;
}
