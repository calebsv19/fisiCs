#include <stdint.h>

typedef struct U8Color4 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} U8Color4;

unsigned u8_color_fold_external(U8Color4 c) {
    return ((unsigned)c.r << 24) |
           ((unsigned)c.g << 16) |
           ((unsigned)c.b << 8) |
           (unsigned)c.a;
}

U8Color4 u8_color_passthrough_external(U8Color4 c) {
    return c;
}
