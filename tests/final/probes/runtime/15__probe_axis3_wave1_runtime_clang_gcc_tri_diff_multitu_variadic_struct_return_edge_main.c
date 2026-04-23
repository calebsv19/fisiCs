#include <stdio.h>

struct Axis3W1Wide {
    long double lo;
    long double hi;
    int lane;
};

struct Axis3W1Wide axis3_w1_wide_collect(
    long double seed_lo,
    long double seed_hi,
    int lane,
    int count,
    ...
);
unsigned axis3_w1_wide_digest(struct Axis3W1Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W1Wide last;
    int i;

    last.lo = 0.0L;
    last.hi = 0.0L;
    last.lane = 0;

    for (i = 0; i < 7; ++i) {
        last = axis3_w1_wide_collect(
            1.0L + (long double)i * 0.25L,
            0.75L + (long double)i * 0.125L,
            i + 2,
            7,
            0, 0.5L + (long double)i * 0.0625L,
            1, 9 + i * 3,
            0, 1.125L + (long double)i * 0.03125L,
            1, 4 - i,
            0, 2.0L + (long double)i * 0.015625L,
            1, 17 + i
        );

        acc ^= axis3_w1_wide_digest(last, (unsigned)(i * 23 + 7));
        acc = rotl32(acc + (unsigned)(i * 19 + 11), 9u);
    }

    printf("%u %u\n", acc, axis3_w1_wide_digest(last, 211u));
    return 0;
}
