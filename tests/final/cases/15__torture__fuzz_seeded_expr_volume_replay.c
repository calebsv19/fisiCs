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

static unsigned eval_expr(unsigned seed, unsigned i, unsigned lane) {
    unsigned a = step(seed ^ (i * 3u + 1u) ^ lane);
    unsigned b = step(a + i * 11u + lane * 17u);
    unsigned c = step(b ^ 0x9e3779b9u);
    unsigned d = step(c + 0x7f4a7c15u + lane * 29u);
    unsigned e = step(d ^ (a + c + i * 7u));

    unsigned lhs = ((a + (b << 1u)) ^ (c >> 3u)) + ((a & b) | (~c));
    unsigned rhs = (d * (c | 1u)) + ((a % 97u) * (b % 89u)) + (e ^ (e >> 9u));
    unsigned mix = (lhs ^ rhs) + ((a < b) ? (c ^ d) : (c + d));
    return rotl32(mix ^ e, (i + lane) & 15u) ^ (a >> ((i + lane) & 7u));
}

int main(void) {
    unsigned hash_a = 2166136261u;
    unsigned hash_b = 2166136261u;
    unsigned seed = 0x00c0ffeeu;

    for (unsigned round = 0u; round < 96u; ++round) {
        seed = step(seed + round * 13u);
        unsigned acc = seed ^ (round * 17u + 5u);
        unsigned lane_seed = seed ^ (round * 31u + 19u);

        for (unsigned lane = 0u; lane < 3u; ++lane) {
            unsigned local = lane_seed ^ (lane * 0x9e37u);
            for (unsigned i = 0u; i < 64u; ++i) {
                unsigned v = eval_expr(local ^ acc, i + round, lane);
                local = rotl32(local ^ v, (i + lane) & 11u) + (v % 251u);
                acc ^= rotl32(v + local + i, (round + lane) & 7u);
                hash_a = fnv1a_u32(hash_a, v ^ local ^ i ^ lane);
            }
            hash_b = fnv1a_u32(hash_b, local ^ lane ^ round);
            lane_seed = step(lane_seed ^ local);
        }

        hash_a = fnv1a_u32(hash_a, acc ^ seed ^ round);
        hash_b = fnv1a_u32(hash_b, lane_seed ^ acc);
    }

    printf("%u %u\n", hash_a, hash_b);
    return 0;
}
