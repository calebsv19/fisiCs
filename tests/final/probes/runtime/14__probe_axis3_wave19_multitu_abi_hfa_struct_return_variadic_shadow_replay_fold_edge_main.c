#include <stdio.h>

struct Axis3W19Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W19Hfa axis3_w19_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned shadow,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w19_hfa_digest(struct Axis3W19Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x6b54a31fu;
    struct Axis3W19Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 25; ++i) {
        last = axis3_w19_hfa_collect(
            1.15625f + (float)i * 0.072265625f,
            (unsigned)(269 + i * 41),
            (unsigned)(19 + i * 37),
            (unsigned)(17 + i * 23),
            23,
            1, 61 + i * 13,
            0, 1.03125 + (double)i * 0.1171875,
            2, 173u + (unsigned)i * 29u,
            0, 2.296875 + (double)i * 0.05859375,
            1, 83 - i,
            2, 239u + (unsigned)i * 19u,
            0, 3.265625 + (double)i * 0.029296875,
            1, 97 + i * 2,
            2, 307u + (unsigned)i * 17u,
            0, 3.984375 + (double)i * 0.0146484375,
            1, 121 + i,
            2, 359u + (unsigned)i * 13u,
            0, 4.5625 + (double)i * 0.00732421875,
            1, 149 + i * 3,
            2, 401u + (unsigned)i * 11u,
            0, 4.96875 + (double)i * 0.003662109375,
            1, 177 - i,
            2, 443u + (unsigned)i * 7u,
            0, 5.28125 + (double)i * 0.0018310546875,
            1, 199 + i * 2,
            2, 487u + (unsigned)i * 5u,
            0, 5.546875 + (double)i * 0.00091552734375,
            1, 223 + i
        );

        acc ^= axis3_w19_hfa_digest(last, (unsigned)(i * 101 + 59));
        acc = rotl32(acc + (unsigned)(i * 71 + 53), 7u);
    }

    printf("%u %u\n", acc, axis3_w19_hfa_digest(last, 1019u));
    return 0;
}
