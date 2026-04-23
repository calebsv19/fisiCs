#include <stdio.h>

struct Axis3W9Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W9Hfa axis3_w9_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w9_hfa_digest(struct Axis3W9Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W9Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 13; ++i) {
        last = axis3_w9_hfa_collect(
            0.9375f + (float)i * 0.1328125f,
            (unsigned)(67 + i * 19),
            (unsigned)(13 + i * 15),
            (unsigned)(5 + i * 11),
            15,
            2, 37u + (unsigned)i * 17u,
            0, 0.8125 + (double)i * 0.171875,
            1, 21 + i * 5,
            0, 1.75 + (double)i * 0.0859375,
            2, 79u + (unsigned)i * 13u,
            1, 35 - i,
            0, 2.625 + (double)i * 0.04296875,
            1, 47 + i,
            2, 113u + (unsigned)i * 9u,
            0, 3.4375 + (double)i * 0.021484375,
            2, 149u + (unsigned)i * 7u,
            1, 63 + i * 2,
            0, 4.125 + (double)i * 0.0107421875,
            1, 81 + i,
            2, 193u + (unsigned)i * 5u
        );

        acc ^= axis3_w9_hfa_digest(last, (unsigned)(i * 47 + 23));
        acc = rotl32(acc + (unsigned)(i * 41 + 11), 7u);
    }

    printf("%u %u\n", acc, axis3_w9_hfa_digest(last, 431u));
    return 0;
}
