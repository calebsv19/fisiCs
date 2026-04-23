#include <stdio.h>

typedef unsigned (*PermFn)(unsigned x, unsigned y);

PermFn perm_pick_symbol(unsigned idx);
unsigned perm_symbol_count(void);

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

static unsigned run_permutation(unsigned base, unsigned pass) {
    static const unsigned perm_a[8] = {3u, 0u, 5u, 1u, 4u, 2u, 7u, 6u};
    static const unsigned perm_b[8] = {6u, 7u, 2u, 4u, 1u, 5u, 0u, 3u};
    const unsigned* perm = (pass & 1u) ? perm_b : perm_a;
    unsigned count = perm_symbol_count();
    unsigned h = 2166136261u;
    unsigned acc = base * 17u + (pass * 29u + 5u);
    unsigned i;

    for (i = 0u; i < 8u; ++i) {
        PermFn fn = perm_pick_symbol(perm[i] % count);
        acc = fn(acc + i, base + perm[i] * 7u + pass);
        h = fnv1a_u32(h, acc ^ (i * 131u + perm[i]));
    }

    return fnv1a_u32(h, acc ^ (pass * 19u + 3u));
}

int main(void) {
    unsigned p0a = run_permutation(13u, 0u);
    unsigned p0b = run_permutation(13u, 0u);
    unsigned p1 = run_permutation(23u, 1u);
    unsigned p2 = run_permutation(31u, 2u);

    if (p0a != p0b) {
        printf("repeatability-fail %u %u\n", p0a, p0b);
        return 17;
    }

    printf("%u %u %u\n", p0a, p1, p2);
    return 0;
}
