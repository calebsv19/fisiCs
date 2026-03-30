#include <stdio.h>

static unsigned step(unsigned x) {
    return x * 1664525u + 1013904223u;
}

static unsigned rotl32(unsigned v, unsigned s) {
    s &= 31u;
    if (s == 0u) {
        return v;
    }
    return (v << s) | (v >> (32u - s));
}

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

static unsigned eval_expr(unsigned seed, unsigned i) {
    unsigned a = step(seed ^ (i * 3u + 1u));
    unsigned b = step(a + i * 11u);
    unsigned c = step(b ^ 0x9e3779b9u);
    unsigned d = step(c + 0x7f4a7c15u);

    unsigned lhs = ((a + (b << 1)) ^ (c >> 3)) + ((a & b) | (~c));
    unsigned rhs = (d * (c | 1u)) + ((a % 97u) * (b % 89u));
    unsigned mix = (lhs ^ rhs) + ((a < b) ? (c ^ d) : (c + d));
    return rotl32(mix, i & 15u) ^ (a >> (i & 7u));
}

int main(void) {
    unsigned hash_a = 2166136261u;
    unsigned hash_b = 2166136261u;
    unsigned seed = 0x00c0ffeeu;

    for (unsigned round = 0; round < 64u; ++round) {
        seed = step(seed + round * 13u);
        unsigned acc = seed ^ (round * 17u + 5u);
        for (unsigned i = 0; i < 32u; ++i) {
            unsigned v = eval_expr(seed ^ acc, i + round);
            acc = rotl32(acc ^ v, (i + round) & 7u) + (v % 251u);
            hash_a = fnv1a_u32(hash_a, v ^ acc ^ i);
        }
        hash_b = fnv1a_u32(hash_b, acc ^ seed ^ round);
    }

    printf("%u %u\n", hash_a, hash_b);
    return 0;
}
