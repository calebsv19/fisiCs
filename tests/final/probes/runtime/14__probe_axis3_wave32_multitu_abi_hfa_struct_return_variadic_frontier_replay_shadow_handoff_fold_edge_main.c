#include <stdio.h>

struct Axis3W32Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W32Hfa axis3_w32_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned replay,
    unsigned shadow,
    unsigned handoff,
    int count,
    ...
);
unsigned axis3_w32_hfa_digest(struct Axis3W32Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x63b7c12du;
    struct Axis3W32Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 35; ++i) {
        last = axis3_w32_hfa_collect(
            1.515625f + (float)i * 0.05029296875f,
            (unsigned)(43 + i * 19),
            (unsigned)(29 + i * 31),
            (unsigned)(83 + i * 17),
            (unsigned)(17 + i * 23),
            23,
            2, 241u + (unsigned)i * 29u,
            0, 1.0859375 + (double)i * 0.072265625,
            1, 99 + i * 4,
            0, 1.7421875 + (double)i * 0.0361328125,
            2, 293u + (unsigned)i * 23u,
            1, 127 - i,
            0, 2.3046875 + (double)i * 0.01806640625,
            2, 347u + (unsigned)i * 19u,
            1, 153 + i * 2,
            0, 2.7890625 + (double)i * 0.009033203125,
            2, 401u + (unsigned)i * 17u,
            1, 181 + i,
            0, 3.1796875 + (double)i * 0.0045166015625,
            2, 449u + (unsigned)i * 13u,
            1, 211 + i * 3,
            0, 3.5234375 + (double)i * 0.00225830078125,
            2, 503u + (unsigned)i * 11u,
            1, 239 - i,
            0, 3.8046875 + (double)i * 0.001129150390625,
            2, 557u + (unsigned)i * 7u,
            1, 263 + i * 2,
            0, 4.0390625 + (double)i * 0.0005645751953125,
            2, 601u + (unsigned)i * 5u,
            1, 281 + i
        );

        acc ^= axis3_w32_hfa_digest(last, (unsigned)(i * 179 + 61));
        acc = rotl32(acc + (unsigned)(i * 103 + 47), 11u);
    }

    printf("%u %u\n", acc, axis3_w32_hfa_digest(last, 2087u));
    return 0;
}
