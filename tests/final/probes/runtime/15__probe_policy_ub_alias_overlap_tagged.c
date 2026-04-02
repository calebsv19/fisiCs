#include <stdio.h>

static unsigned fold(unsigned *restrict p, unsigned *restrict q, unsigned *restrict r) {
    *p = *p + 0x11u;
    *q = (*q ^ 0x22u) + 3u;
    *r = (*r << 1) ^ 0x55u;
    return *p + (*q * 3u) + (*r * 5u);
}

int main(void) {
    unsigned value = 0x1234u;
    unsigned total = fold(&value, &value, &value);
    printf("%u %u\n", value, total);
    return 0;
}
