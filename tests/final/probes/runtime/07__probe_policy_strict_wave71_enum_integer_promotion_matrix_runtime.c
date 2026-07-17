#include <stdio.h>

enum SignedLevel {
    LEVEL_NEG = -8,
    LEVEL_ZERO = 0,
    LEVEL_POS = 17
};

enum UnsignedMask {
    MASK_LOW = 3u,
    MASK_HIGH = 200u
};

static long mix(enum SignedLevel level, unsigned short width, signed char adjust) {
    unsigned int widened = width + (unsigned char)adjust;
    long signed_mix = (long)(level + adjust);
    return signed_mix + (long)(widened & MASK_HIGH);
}

int main(void) {
    signed char neg = -5;
    unsigned char large = 250u;
    unsigned short wrapped = (unsigned short)(large + (unsigned char)10u);
    int enum_cmp = (LEVEL_POS + neg) > (int)MASK_LOW;
    unsigned int ternary = enum_cmp ? (unsigned int)LEVEL_POS : MASK_HIGH;
    long total = mix(LEVEL_NEG, wrapped, 9);

    printf("%d %u %u %ld %d\n",
           enum_cmp,
           (unsigned int)wrapped,
           ternary,
           total,
           (int)((LEVEL_ZERO - 1) < MASK_HIGH));
    return 0;
}
