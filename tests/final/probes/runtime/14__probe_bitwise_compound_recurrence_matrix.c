#include <stdio.h>

static unsigned step(unsigned x, unsigned y) {
    x ^= y;
    x += (y << 3);
    x -= (x >> 5);
    x |= 0x55AAu;
    x &= 0xFFFFu;
    return x;
}

int main(void) {
    unsigned x = 0x1234u;
    unsigned y = 0x00F1u;
    unsigned acc = 0;

    for (int i = 0; i < 7; ++i) {
        x = step(x, y + (unsigned)i);
        acc ^= (x >> (i & 7));
    }

    printf("%u %u\n", x, acc);
    return 0;
}

