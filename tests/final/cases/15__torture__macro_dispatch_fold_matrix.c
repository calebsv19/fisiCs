#include <stdio.h>

#define BIND3(a, b, c) ((a) + (b) + (c))
#define RSHIFT(v, s) ((unsigned)(v) >> ((s) & 31u))
#define LSHIFT(v, s) ((unsigned)(v) << (((s) & 15u) + 1u))

static unsigned rotate(unsigned v, unsigned s) {
    unsigned k = s & 31u;
    return (v << k) | (v >> ((32u - k) & 31u));
}

static unsigned fold_stage(unsigned base, unsigned a, unsigned b, unsigned c) {
    unsigned x = BIND3(a, b, c) + 0x9e3779b9u;
    unsigned y = rotate(base ^ x, a) + (b * 2654435761u);
    unsigned z = (x ^ y) * 0x45d9f3bu + (c << 2u);
    return rotate((x + y + z), b) ^ (RSHIFT(z, c) + LSHIFT(a, b));
}

int main(void) {
    unsigned out = 0u;
    unsigned seed = 0x89abcdefu;

    for (unsigned i = 0u; i < 64u; ++i) {
        unsigned a = (seed ^ (i * 37u)) + 7u;
        unsigned b = (seed >> ((i & 3u) + 1u)) + 11u;
        unsigned c = (seed + i * 131u) ^ (i << 4u);
        seed = rotate(seed + a + b, c) ^ (a * 3u + b * 5u + c);
        out ^= fold_stage(seed, a, b, c);

        if ((i & 7u) == 0u) {
            out = (out * 16777619u) ^ (seed + c);
        }
    }

    printf("%u\n", out);
    return 0;
}
