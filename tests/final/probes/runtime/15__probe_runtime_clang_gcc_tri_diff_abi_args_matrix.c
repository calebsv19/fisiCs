#include <stdio.h>

typedef struct {
    unsigned a;
    unsigned b;
    unsigned c;
} Triple;

static unsigned fold9(
    unsigned a0, unsigned a1, unsigned a2,
    unsigned a3, unsigned a4, unsigned a5,
    unsigned a6, unsigned a7, unsigned a8
) {
    unsigned acc = 0x9e3779b9u;
    acc ^= a0 + a3 * 3u;
    acc ^= a1 + a4 * 5u;
    acc ^= a2 + a5 * 7u;
    acc ^= a6 * 11u + a7 * 13u + a8 * 17u;
    acc = (acc << 9) | (acc >> 23);
    return acc ^ (acc >> 15);
}

static unsigned fold_mix(
    unsigned x0, unsigned x1, unsigned x2, unsigned x3,
    unsigned x4, unsigned x5, unsigned x6, unsigned x7,
    Triple t0, Triple t1
) {
    unsigned acc = fold9(x0, x1, x2, x3, x4, x5, x6, x7, t0.a + t1.a);
    acc ^= t0.b * 19u + t1.b * 23u;
    acc ^= t0.c * 29u + t1.c * 31u;
    return acc + (x0 ^ x7) + t0.a * 37u + t1.c * 41u;
}

int main(void) {
    Triple t0 = {3u, 5u, 7u};
    Triple t1 = {11u, 13u, 17u};
    unsigned v0 = fold_mix(1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, t0, t1);
    unsigned v1 = fold_mix(8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, t1, t0);
    unsigned v2 = fold9(v0, v1, t0.a, t0.b, t0.c, t1.a, t1.b, t1.c, 97u);
    printf("%u %u\n", v2, v0 ^ (v1 + v2 * 3u));
    return 0;
}
