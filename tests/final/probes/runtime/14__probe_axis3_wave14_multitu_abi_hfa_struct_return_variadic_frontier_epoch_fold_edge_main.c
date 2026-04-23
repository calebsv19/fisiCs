#include <stdio.h>

struct Axis3W14Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W14Hfa axis3_w14_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w14_hfa_digest(struct Axis3W14Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W14Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 17; ++i) {
        last = axis3_w14_hfa_collect(
            1.09375f + (float)i * 0.091796875f,
            (unsigned)(149 + i * 37),
            (unsigned)(11 + i * 23),
            (unsigned)(5 + i * 21),
            21,
            2, 79u + (unsigned)i * 31u,
            0, 0.90625 + (double)i * 0.13671875,
            1, 37 + i * 13,
            0, 2.15625 + (double)i * 0.068359375,
            2, 173u + (unsigned)i * 19u,
            1, 61 - i,
            0, 3.03125 + (double)i * 0.0341796875,
            2, 223u + (unsigned)i * 17u,
            1, 83 + i,
            0, 3.6875 + (double)i * 0.01708984375,
            1, 101 + i * 2,
            2, 257u + (unsigned)i * 13u,
            0, 4.25 + (double)i * 0.008544921875,
            1, 127 + i,
            2, 293u + (unsigned)i * 11u,
            0, 4.71875 + (double)i * 0.0042724609375,
            1, 149 + i * 3,
            2, 337u + (unsigned)i * 7u,
            0, 5.03125 + (double)i * 0.00213623046875,
            1, 167 - i,
            2, 359u + (unsigned)i * 5u
        );

        acc ^= axis3_w14_hfa_digest(last, (unsigned)(i * 71 + 43));
        acc = rotl32(acc + (unsigned)(i * 59 + 29), 11u);
    }

    printf("%u %u\n", acc, axis3_w14_hfa_digest(last, 677u));
    return 0;
}
