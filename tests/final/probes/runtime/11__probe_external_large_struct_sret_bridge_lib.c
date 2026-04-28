typedef struct ProbeStats {
    unsigned long long a;
    unsigned long long b;
    unsigned long long c;
    unsigned long long d;
    unsigned long long e;
} ProbeStats;

ProbeStats probe_external_large_struct_sret_bridge(const char *tag, int *seen) {
    ProbeStats stats = {11ULL, 22ULL, 33ULL, 44ULL, 55ULL};
    if (tag && tag[0] == 'o' && tag[1] == 'k' && tag[2] == '\0' && seen) {
        *seen = 77;
    }
    return stats;
}
