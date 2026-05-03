#include <stdio.h>

unsigned sir_seed(void);
void sir_reset(unsigned salt);
unsigned sir_step(unsigned lane, unsigned weight);
unsigned sir_snapshot(void);

static unsigned fnv1a_u32(unsigned h, unsigned v) {
    h ^= (v & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 8) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 16) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 24) & 0xFFu);
    h *= 16777619u;
    return h;
}

static unsigned run_pattern(const unsigned char* order, unsigned count, unsigned salt) {
    unsigned h = 2166136261u;
    unsigned acc = sir_seed() ^ (salt * 131u + 17u);
    unsigned i;

    sir_reset(salt);
    for (i = 0u; i < count; ++i) {
        unsigned step = sir_step(order[i], i + 3u);
        acc ^= step + (i * 97u + 11u);
        h = fnv1a_u32(h, acc ^ sir_snapshot());
        h = fnv1a_u32(h, step + (salt * 53u + i));
    }

    return fnv1a_u32(h, acc ^ sir_snapshot());
}

int main(void) {
    static const unsigned char pattern0[] = {1u, 3u, 2u, 0u, 4u, 2u, 1u, 3u};
    static const unsigned char pattern1[] = {4u, 0u, 1u, 4u, 2u, 3u, 0u, 2u};
    static const unsigned char pattern2[] = {2u, 2u, 3u, 1u, 0u, 4u, 1u, 4u};
    unsigned h0 = run_pattern(pattern0, 8u, 5u);
    unsigned h1 = run_pattern(pattern1, 8u, 7u);
    unsigned h2 = run_pattern(pattern2, 8u, 11u);
    unsigned repeat = run_pattern(pattern1, 8u, 7u);

    if (repeat != h1) {
        printf("static-init-repeatability-fail %u %u\n", h1, repeat);
        return 31;
    }

    printf("%u %u %u\n", h0, h1, h2);
    return 0;
}
