#include <stdio.h>

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

static unsigned run_program(unsigned seed) {
    static const unsigned opcodes[] = {0u, 1u, 2u, 3u, 1u, 0u, 2u, 3u};
    static const unsigned args[] = {7u, 3u, 11u, 5u, 13u, 2u, 17u, 19u};
    unsigned acc = seed * 97u + 23u;
    unsigned trace = 2166136261u;
    int i;

    for (i = 0; i < (int)(sizeof(opcodes) / sizeof(opcodes[0])); ++i) {
        unsigned op = opcodes[i];
        unsigned a = args[i] + (unsigned)i;
        switch (op) {
            case 0u: acc = (acc + a) ^ (a << 1); break;
            case 1u: acc = (acc * (a | 1u)) + (a ^ 0x9e37u); break;
            case 2u: acc = (acc ^ (a * 33u)) + (acc >> 3); break;
            default: acc = (acc + (a * a + 7u)) ^ (acc << 5); break;
        }
        trace = fnv1a_u32(trace, acc ^ (unsigned)i);
    }

    return fnv1a_u32(trace, acc);
}

int main(void) {
    unsigned agg_a = 2166136261u;
    unsigned agg_b = 2166136261u;
    unsigned check = 0u;
    unsigned seed;

    for (seed = 1u; seed <= 9u; ++seed) {
        unsigned first = run_program(seed);
        unsigned second = run_program(seed);
        if (first != second) {
            printf("repeatability-fail %u %u %u\n", seed, first, second);
            return 11;
        }
        agg_a = fnv1a_u32(agg_a, first);
        agg_b = fnv1a_u32(agg_b, run_program(seed + 101u));
        check ^= (first + seed * 31u);
    }

    printf("%u %u %u\n", agg_a, agg_b, check);
    return 0;
}
