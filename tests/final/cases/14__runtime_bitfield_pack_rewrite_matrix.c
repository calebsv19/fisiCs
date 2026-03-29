#include <stdio.h>

struct PackBits {
    unsigned a : 3;
    unsigned b : 7;
    unsigned c : 10;
    unsigned d : 12;
};

static struct PackBits step(struct PackBits in, unsigned seed) {
    in.a = (in.a + (seed & 7u)) & 7u;
    in.b = (in.b ^ (seed * 5u)) & 127u;
    in.c = (in.c + ((seed << 1) ^ 0x55u)) & 1023u;
    in.d = (in.d + ((seed * 9u) & 0x3FFu)) & 4095u;
    return in;
}

int main(void) {
    struct PackBits p = {2u, 17u, 300u, 900u};
    unsigned checksum = 0;
    for (unsigned i = 0; i < 8u; ++i) {
        p = step(p, i * 3u + 1u);
        checksum = checksum * 257u + p.a + p.b * 16u + p.c * 256u + p.d * 2048u;
    }
    printf("%u %u %u %u %u\n", p.a, p.b, p.c, p.d, checksum);
    return 0;
}
