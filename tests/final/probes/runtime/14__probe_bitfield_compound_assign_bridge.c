#include <stdio.h>

struct UnsignedBits {
    unsigned int lo : 4;
    unsigned int mid : 4;
    unsigned int hi : 8;
};

int main(void) {
    struct UnsignedBits b = {3u, 12u, 200u};
    unsigned score = 0u;

    for (unsigned i = 0; i < 6u; ++i) {
        b.lo = (b.lo + (i + 5u)) & 15u;
        b.mid = (b.mid ^ ((i * 3u + 1u) & 15u)) & 15u;
        b.hi = (b.hi >> 1) & 255u;
        b.hi = (b.hi + b.lo + b.mid) & 255u;

        score = score * 131u + b.lo + b.mid * 16u + b.hi * 256u;
    }

    printf("%u %u %u %u\n", b.lo, b.mid, b.hi, score);
    return 0;
}
