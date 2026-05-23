#include <stdio.h>

struct Axis3W23Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W23Hfa axis3_w23_hfa_collect(
    float seed,
    unsigned replay,
    unsigned checkpoint,
    unsigned handoff,
    int count,
    ...
);
unsigned axis3_w23_hfa_digest(struct Axis3W23Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x63b17d25u;
    struct Axis3W23Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 29; ++i) {
        last = axis3_w23_hfa_collect(
            1.3984375f + (float)i * 0.0615234375f,
            (unsigned)(41 + i * 37),
            (unsigned)(53 + i * 19),
            (unsigned)(67 + i * 13),
            25,
            1, 71 + i * 5,
            0, 1.046875 + (double)i * 0.09375,
            2, 211u + (unsigned)i * 31u,
            0, 2.03125 + (double)i * 0.046875,
            1, 93 - i,
            2, 257u + (unsigned)i * 23u,
            0, 2.703125 + (double)i * 0.0234375,
            1, 119 + i * 2,
            2, 311u + (unsigned)i * 19u,
            0, 3.265625 + (double)i * 0.01171875,
            1, 147 + i,
            2, 359u + (unsigned)i * 17u,
            0, 3.703125 + (double)i * 0.005859375,
            1, 173 + i * 3,
            2, 401u + (unsigned)i * 13u,
            0, 4.0625 + (double)i * 0.0029296875,
            1, 197 - i,
            2, 449u + (unsigned)i * 11u,
            0, 4.34375 + (double)i * 0.00146484375,
            1, 223 + i * 2,
            2, 503u + (unsigned)i * 7u,
            0, 4.578125 + (double)i * 0.000732421875,
            1, 251 + i
        );

        acc ^= axis3_w23_hfa_digest(last, (unsigned)(i * 127 + 73));
        acc = rotl32(acc + (unsigned)(i * 89 + 31), 15u);
    }

    printf("%u %u\n", acc, axis3_w23_hfa_digest(last, 1301u));
    return 0;
}
