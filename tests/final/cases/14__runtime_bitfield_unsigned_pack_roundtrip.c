#include <stdio.h>

struct Bits {
    unsigned a : 3;
    unsigned b : 5;
    unsigned c : 6;
    unsigned d : 2;
};

int main(void) {
    struct Bits x = {0, 0, 0, 0};
    x.a = 5u;
    x.b = 17u;
    x.c = 41u;
    x.d = 2u;

    unsigned mix = x.a + x.b * 10u + x.c * 100u + x.d * 1000u;
    printf("%u %u %u %u %u\n", x.a, x.b, x.c, x.d, mix);
    return 0;
}
