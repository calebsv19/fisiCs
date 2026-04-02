#include <stdio.h>

static unsigned long long recurse_pressure_iii(int depth, unsigned long long seed) {
    unsigned long long frame[32];
    unsigned long long local = seed ^ (unsigned long long)(depth * 211u + 43u);

    for (int i = 0; i < 32; ++i) {
        frame[i] = local + (unsigned long long)(i * 29u + depth * 5u);
        local ^= (frame[i] << ((unsigned)i & 7u));
        local = (local * 6364136223846793005ULL) + 1442695040888963407ULL;
    }

    if (depth <= 0) {
        return local ^ frame[9] ^ frame[21] ^ frame[29];
    }

    {
        unsigned long long next_seed =
            (seed * 11400714819323198485ULL) ^ frame[(depth * 7) % 32];
        unsigned long long child = recurse_pressure_iii(depth - 1, next_seed);
        return child ^ local ^ frame[(depth + 13) % 32];
    }
}

int main(void) {
    unsigned long long out = recurse_pressure_iii(360, 0xD00DCAFE7711ULL);
    printf("%llu\n", out);
    return 0;
}
