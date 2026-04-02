#include <stdio.h>

static unsigned fold_window(const unsigned *base, unsigned n, unsigned salt) {
    unsigned acc = 0x811c9dc5u ^ salt;
    unsigned i = 0u;
    for (i = 0u; i < n; ++i) {
        const unsigned *p = base + i;
        const unsigned *q = base + (n - 1u - i);
        unsigned mix = (*p * (i + 3u)) ^ (*q + (salt << (i & 7u)));
        acc ^= mix + (acc >> 9);
        acc *= 16777619u;
    }
    return acc ^ (acc >> 15);
}

int main(void) {
    static const unsigned data[] = {
        10u, 20u, 30u, 40u, 50u, 60u, 70u, 80u, 90u, 100u
    };
    unsigned a = fold_window(data, 10u, 1u);
    unsigned b = fold_window(data + 2, 6u, 5u);
    unsigned c = fold_window(data + 1, 8u, 9u);
    printf("%u %u\n", a ^ b, c + (a * 3u) + (b * 7u));
    return 0;
}
