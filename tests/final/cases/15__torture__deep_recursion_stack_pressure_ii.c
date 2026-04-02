#include <stdio.h>

static unsigned long long recurse_pressure_ii(int depth, unsigned long long seed) {
    unsigned long long frame[28];
    unsigned long long local = seed ^ (unsigned long long)(depth * 197u + 31u);

    for (int i = 0; i < 28; ++i) {
        frame[i] = local + (unsigned long long)(i * 23u + depth * 3u);
        local ^= (frame[i] << ((unsigned)i & 7u));
        local = (local * 6364136223846793005ULL) + 1442695040888963407ULL;
    }

    if (depth <= 0) {
        return local ^ frame[7] ^ frame[19] ^ frame[25];
    }

    {
        unsigned long long next_seed =
            (seed * 11400714819323198485ULL) ^ frame[(depth * 5) % 28];
        unsigned long long child = recurse_pressure_ii(depth - 1, next_seed);
        return child ^ local ^ frame[(depth + 11) % 28];
    }
}

int main(void) {
    unsigned long long out = recurse_pressure_ii(300, 0xC0FFEE55AA11ULL);
    printf("%llu\n", out);
    return 0;
}
