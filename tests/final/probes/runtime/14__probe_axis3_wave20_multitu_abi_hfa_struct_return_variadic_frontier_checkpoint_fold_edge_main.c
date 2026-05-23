#include <stdio.h>

struct Axis3W20Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W20Hfa axis3_w20_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned checkpoint,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w20_hfa_digest(struct Axis3W20Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x71c63a4bu;
    struct Axis3W20Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 27; ++i) {
        last = axis3_w20_hfa_collect(
            1.21875f + (float)i * 0.0693359375f,
            (unsigned)(211 + i * 47),
            (unsigned)(37 + i * 31),
            (unsigned)(23 + i * 19),
            24,
            2, 163u + (unsigned)i * 31u,
            0, 1.015625 + (double)i * 0.109375,
            1, 57 + i * 11,
            0, 2.171875 + (double)i * 0.0546875,
            2, 211u + (unsigned)i * 23u,
            1, 79 - i,
            0, 2.9375 + (double)i * 0.02734375,
            2, 257u + (unsigned)i * 19u,
            1, 101 + i * 2,
            0, 3.5625 + (double)i * 0.013671875,
            2, 313u + (unsigned)i * 17u,
            1, 129 + i,
            0, 4.03125 + (double)i * 0.0068359375,
            2, 359u + (unsigned)i * 13u,
            1, 151 + i * 3,
            0, 4.421875 + (double)i * 0.00341796875,
            2, 401u + (unsigned)i * 11u,
            1, 173 - i,
            0, 4.734375 + (double)i * 0.001708984375,
            2, 443u + (unsigned)i * 7u,
            1, 197 + i * 2,
            0, 4.96875 + (double)i * 0.0008544921875,
            2, 487u + (unsigned)i * 5u,
            1, 223 + i
        );

        acc ^= axis3_w20_hfa_digest(last, (unsigned)(i * 107 + 61));
        acc = rotl32(acc + (unsigned)(i * 73 + 41), 9u);
    }

    printf("%u %u\n", acc, axis3_w20_hfa_digest(last, 1091u));
    return 0;
}
