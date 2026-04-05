#include <stdint.h>
#include <stdio.h>

typedef struct U8Color4 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} U8Color4;

static U8Color4 pass_through(U8Color4 in) {
    return in;
}

static unsigned fold(U8Color4 c) {
    return ((unsigned)c.r << 24) |
           ((unsigned)c.g << 16) |
           ((unsigned)c.b << 8) |
           (unsigned)c.a;
}

int main(void) {
    U8Color4 in = {230u, 231u, 232u, 233u};
    U8Color4 out = pass_through(in);

    unsigned cast_r = (unsigned)in.r;
    unsigned add = in.r + 5u;
    int gt = (in.r > 200u) ? 1 : 0;
    unsigned fold_in = fold(in);
    unsigned fold_out = fold(out);

    printf("U8_OK cast=%u add=%u gt=%d out=%u,%u,%u,%u fold=%u/%u\n",
           cast_r,
           add,
           gt,
           (unsigned)out.r,
           (unsigned)out.g,
           (unsigned)out.b,
           (unsigned)out.a,
           fold_in,
           fold_out);
    return 0;
}
