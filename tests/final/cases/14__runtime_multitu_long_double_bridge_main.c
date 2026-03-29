#include <stdio.h>

struct ld_pair {
    long double first;
    long double second;
};

struct ld_pair ld_pair_make(long double a, long double b);
long double ld_pair_second(struct ld_pair pair);

static unsigned fold_bytes(const unsigned char *bytes, int n) {
    unsigned hash = 2166136261u;
    for (int i = 0; i < n; ++i) {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

int main(void) {
    struct ld_pair pair = ld_pair_make(1.0L, -2.0L);
    long double second = ld_pair_second(pair);
    unsigned h_pair = fold_bytes((const unsigned char *)&pair, (int)sizeof(pair));
    unsigned h_second = fold_bytes((const unsigned char *)&second, (int)sizeof(second));
    printf("%zu %u %u\n", sizeof(long double), h_pair, h_second);
    return 0;
}
