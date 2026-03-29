#include <stdio.h>

struct BoundaryBits {
    unsigned a : 9;
    unsigned b : 9;
    unsigned c : 9;
    unsigned d : 5;
};

int main(void) {
    struct BoundaryBits s = {17u, 200u, 300u, 11u};
    for (unsigned i = 0; i < 9u; ++i) {
        unsigned seed = i * 5u + 3u;
        s.a = (unsigned)((s.a + (seed & 0x1Fu)) & 0x1FFu);
        s.b = (unsigned)((s.b ^ (seed * 7u)) & 0x1FFu);
        s.c = (unsigned)((s.c + ((seed << 2) ^ 0x55u)) & 0x1FFu);
        s.d = (unsigned)((s.d + (seed & 0x0Fu)) & 0x1Fu);
    }

    unsigned checksum = 0u;
    checksum = checksum * 131u + s.a;
    checksum = checksum * 131u + s.b;
    checksum = checksum * 131u + s.c;
    checksum = checksum * 131u + s.d;
    checksum = checksum * 131u + (unsigned)sizeof(s);

    printf("%u %u %u %u %zu %u\n", s.a, s.b, s.c, s.d, sizeof(s), checksum);
    return 0;
}
