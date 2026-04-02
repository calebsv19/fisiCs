#include <stdio.h>

typedef struct {
    signed char s;
    unsigned char u;
    unsigned char sh;
} ShiftLane;

static unsigned rotl8(unsigned x, unsigned r) {
    r &= 7u;
    return ((x << r) | (x >> ((8u - r) & 7u))) & 0xFFu;
}

static unsigned fold_lane(const ShiftLane *lane, unsigned bias, unsigned idx) {
    unsigned us = (unsigned)(unsigned char)lane->s;
    unsigned uu = (unsigned)lane->u;
    unsigned sh = (unsigned)(lane->sh & 7u);
    unsigned left = (us << sh) & 0xFFu;
    unsigned right = uu >> sh;
    unsigned mix = rotl8(us ^ uu ^ (bias + idx * 11u), sh);
    return (left ^ (right << 1u) ^ mix ^ (bias * 3u + idx * 7u)) & 0xFFu;
}

int main(void) {
    static const ShiftLane lanes[] = {
        {(signed char)-1, (unsigned char)0x80u, 1u},
        {(signed char)7, (unsigned char)0x55u, 2u},
        {(signed char)-33, (unsigned char)0xA5u, 3u},
        {(signed char)64, (unsigned char)0x11u, 4u},
        {(signed char)-120, (unsigned char)0xFEu, 5u},
        {(signed char)12, (unsigned char)0x33u, 6u}
    };
    unsigned acc0 = 0x9e3779b9u;
    unsigned acc1 = 0x51f15e5u;
    unsigned i;

    for (i = 0u; i < (unsigned)(sizeof(lanes) / sizeof(lanes[0])); ++i) {
        unsigned lane = fold_lane(&lanes[i], acc0 ^ acc1, i);
        acc0 = (acc0 * 131u) ^ (lane + i * 17u + 19u);
        acc1 = ((acc1 << 5u) | (acc1 >> 27u)) + (lane * 257u) + i * 13u;
        acc1 ^= (acc0 >> ((i & 7u) + 1u));
    }

    printf("%u %u\n", acc0, acc0 ^ (acc1 + 97u));
    return 0;
}
