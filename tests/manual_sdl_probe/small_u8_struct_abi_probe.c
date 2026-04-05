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
    return ((unsigned)c.r << 24) | ((unsigned)c.g << 16) | ((unsigned)c.b << 8) | (unsigned)c.a;
}

int main(void) {
    U8Color4 in = { 230u, 231u, 232u, 233u };
    U8Color4 out = pass_through(in);
    printf("in=%u,%u,%u,%u out=%u,%u,%u,%u fold_in=%u fold_out=%u\n",
           (unsigned)in.r, (unsigned)in.g, (unsigned)in.b, (unsigned)in.a,
           (unsigned)out.r, (unsigned)out.g, (unsigned)out.b, (unsigned)out.a,
           fold(in), fold(out));
    return 0;
}
