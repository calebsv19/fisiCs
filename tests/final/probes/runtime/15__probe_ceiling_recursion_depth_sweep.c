#include <stdio.h>

static unsigned long long sweep_rec(int depth, unsigned long long seed, unsigned lane) {
    unsigned long long frame[24];
    unsigned long long x =
        seed ^ (unsigned long long)(depth * 131u + lane * 17u + 41u);

    for (unsigned i = 0u; i < 24u; ++i) {
        frame[i] = x + (unsigned long long)(i * 29u + depth * 7u + lane * 11u);
        x ^= frame[i] + (x << 7u) + (x >> 3u);
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
    }

    if (depth <= 0) {
        return x ^ frame[lane % 24u] ^ frame[(lane + 11u) % 24u];
    }

    {
        unsigned idx = ((unsigned)depth * 5u + lane) % 24u;
        unsigned long long child = sweep_rec(depth - 1, x ^ frame[idx], lane + 1u);
        return child ^ x ^ frame[(idx + 9u) % 24u];
    }
}

int main(void) {
    unsigned long long r0 = sweep_rec(180, 0x13579BDF2468ULL, 1u);
    unsigned long long r1 = sweep_rec(240, 0xACE02468BDF1ULL, 2u);
    unsigned long long r2 = sweep_rec(300, 0x55AA77331199ULL, 3u);
    unsigned long long out0 = r0 ^ (r1 << 1u) ^ (r2 >> 1u);
    unsigned long long out1 = (r0 * 3ULL) ^ (r1 * 5ULL) ^ (r2 + 17ULL);

    printf("%llu %llu\n", out0, out1);
    return 0;
}
