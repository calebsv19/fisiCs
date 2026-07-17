#include <stdio.h>

enum Mode {
    MODE_NEG = -4,
    MODE_LOW = 7,
    MODE_HIGH = 130
};

static long mix_values(enum Mode mode, unsigned short width, signed char adjust) {
    int promoted = mode + adjust;
    unsigned short wrapped = (unsigned short)(width + (unsigned char)MODE_LOW);
    signed char narrowed = (signed char)((unsigned char)250u + MODE_LOW);
    unsigned char widened = (unsigned char)(adjust - MODE_NEG);
    return (long)promoted + (long)wrapped + (long)narrowed + (long)widened;
}

int main(void) {
    signed char negative = -12;
    unsigned char large = 250u;
    unsigned short almost_wrap = 65530u;
    int sum = MODE_HIGH + negative;
    unsigned short wrapped = (unsigned short)(almost_wrap + (unsigned char)MODE_LOW);
    int cmp = (MODE_NEG + large) > MODE_HIGH;
    long total = mix_values(MODE_HIGH, almost_wrap, negative);

    printf("%d %u %d %ld %u\n",
           sum,
           (unsigned int)wrapped,
           cmp,
           total,
           cmp ? (unsigned int)MODE_NEG : (unsigned int)MODE_HIGH);
    return 0;
}
