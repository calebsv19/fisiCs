#include <stdio.h>

struct Bits {
    unsigned lo : 4;
    unsigned mid : 6;
    unsigned hi : 5;
    unsigned flag : 1;
};

static struct Bits mutate(struct Bits in, unsigned seed) {
    in.lo = (in.lo + (seed & 15u)) & 15u;
    in.mid = (in.mid ^ (seed * 3u)) & 63u;
    in.hi = (in.hi + ((seed >> 1) & 31u)) & 31u;
    in.flag = (in.flag ^ (seed & 1u)) & 1u;
    return in;
}

int main(void) {
    struct Bits b = {7u, 35u, 12u, 1u};
    unsigned checksum = 0;
    for (unsigned i = 0; i < 6u; ++i) {
        b = mutate(b, i * 5u + 3u);
        checksum = checksum * 131u + b.lo + b.mid * 16u + b.hi * 1024u + b.flag * 32768u;
    }
    printf("%u %u %u %u %u\n", b.lo, b.mid, b.hi, b.flag, checksum);
    return 0;
}
