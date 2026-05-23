#include <stdio.h>

struct Axis3W21Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W21Hfa axis3_w21_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned checkpoint,
    unsigned frontier,
    int count,
    ...
);
unsigned axis3_w21_hfa_digest(struct Axis3W21Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x58d2f31bu;
    struct Axis3W21Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 26; ++i) {
        last = axis3_w21_hfa_collect(
            1.2734375f + (float)i * 0.0673828125f,
            (unsigned)(29 + i * 41),
            (unsigned)(43 + i * 23),
            (unsigned)(187 + i * 37),
            24,
            1, 63 + i * 9,
            0, 1.09375 + (double)i * 0.1015625,
            2, 181u + (unsigned)i * 31u,
            0, 2.234375 + (double)i * 0.05078125,
            1, 91 - i,
            2, 239u + (unsigned)i * 19u,
            0, 2.96875 + (double)i * 0.025390625,
            1, 107 + i * 2,
            2, 283u + (unsigned)i * 17u,
            0, 3.578125 + (double)i * 0.0126953125,
            1, 137 + i,
            2, 337u + (unsigned)i * 13u,
            0, 4.046875 + (double)i * 0.00634765625,
            1, 163 + i * 3,
            2, 389u + (unsigned)i * 11u,
            0, 4.421875 + (double)i * 0.003173828125,
            1, 181 - i,
            2, 433u + (unsigned)i * 7u,
            0, 4.71875 + (double)i * 0.0015869140625,
            1, 203 + i * 2,
            2, 479u + (unsigned)i * 5u,
            0, 4.953125 + (double)i * 0.00079345703125,
            1, 229 + i
        );

        acc ^= axis3_w21_hfa_digest(last, (unsigned)(i * 109 + 67));
        acc = rotl32(acc + (unsigned)(i * 79 + 37), 11u);
    }

    printf("%u %u\n", acc, axis3_w21_hfa_digest(last, 1163u));
    return 0;
}
