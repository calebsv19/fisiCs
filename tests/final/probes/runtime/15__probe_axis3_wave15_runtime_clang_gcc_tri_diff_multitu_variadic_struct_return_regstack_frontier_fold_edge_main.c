#include <stdio.h>

struct Axis3W15Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W15Wide axis3_w15_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w15_wide_digest(struct Axis3W15Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W15Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 16; ++i) {
        last = axis3_w15_wide_collect(
            1.03125L + (long double)i * 0.109375L,
            0.8125L + (long double)i * 0.0546875L,
            0.609375L + (long double)i * 0.0283203125L,
            (unsigned)(191 + i * 31),
            (unsigned)(11 + i * 23),
            (unsigned)(7 + i * 21),
            23,
            2, 113u + (unsigned)i * 31u,
            0, 1.21875L + (long double)i * 0.1171875L,
            1, 47 + i * 17,
            0, 2.125L + (long double)i * 0.05859375L,
            2, 229u + (unsigned)i * 23u,
            1, 71 - i,
            0, 3.21875L + (long double)i * 0.029296875L,
            1, 89 + i,
            2, 277u + (unsigned)i * 19u,
            0, 3.84375L + (long double)i * 0.0146484375L,
            1, 101 + i * 2,
            2, 317u + (unsigned)i * 17u,
            0, 4.375L + (long double)i * 0.00732421875L,
            2, 353u + (unsigned)i * 13u,
            1, 127 + i,
            0, 4.8125L + (long double)i * 0.003662109375L,
            1, 149 + i * 3,
            2, 397u + (unsigned)i * 11u,
            0, 5.125L + (long double)i * 0.0018310546875L,
            1, 179 - i,
            2, 439u + (unsigned)i * 7u,
            0, 5.40625L + (long double)i * 0.00091552734375L,
            1, 211 + i * 2
        );

        acc ^= axis3_w15_wide_digest(last, (unsigned)(i * 97 + 53));
        acc = rotl32(acc + (unsigned)(i * 71 + 31), 9u);
    }

    printf("%u %u\n", acc, axis3_w15_wide_digest(last, 787u));
    return 0;
}
