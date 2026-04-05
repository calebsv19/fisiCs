#include <stdint.h>
#include <stdio.h>

typedef struct U8Color4 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} U8Color4;

unsigned u8_color_fold_external(U8Color4 c);
U8Color4 u8_color_passthrough_external(U8Color4 c);

int main(void) {
    U8Color4 in = {230u, 231u, 232u, 233u};
    U8Color4 out = u8_color_passthrough_external(in);
    unsigned fold = u8_color_fold_external(in);

    printf("MULTITU_U8 in=%u,%u,%u,%u out=%u,%u,%u,%u fold=%u\n",
           (unsigned)in.r, (unsigned)in.g, (unsigned)in.b, (unsigned)in.a,
           (unsigned)out.r, (unsigned)out.g, (unsigned)out.b, (unsigned)out.a,
           fold);
    return 0;
}
