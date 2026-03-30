#include <stdio.h>

static unsigned long long recurse_pressure(int depth, unsigned long long seed) {
    unsigned long long frame[24];
    unsigned long long local = seed ^ (unsigned long long)(depth * 131u);

    for (int i = 0; i < 24; ++i) {
        frame[i] = local + (unsigned long long)(i * 17u + depth);
        local ^= (frame[i] << ((unsigned)i & 7u));
        local = (local * 6364136223846793005ULL) + 1442695040888963407ULL;
    }

    if (depth <= 0) {
        return local ^ frame[5] ^ frame[17];
    }

    unsigned long long next_seed = (seed * 11400714819323198485ULL) ^ frame[depth % 24];
    unsigned long long child = recurse_pressure(depth - 1, next_seed);
    return child ^ local ^ frame[(depth + 7) % 24];
}

int main(void) {
    unsigned long long out = recurse_pressure(220, 0xC0FFEE1234ULL);
    printf("%llu\n", out);
    return 0;
}
