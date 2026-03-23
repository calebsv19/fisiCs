#include <stdio.h>

typedef struct Bits {
    unsigned a : 5;
    unsigned b : 6;
    unsigned c : 3;
} Bits;

static Bits bounce(Bits x, unsigned k) {
    Bits y = x;
    y.a = (x.a + k) & 31u;
    y.b = (x.b ^ (k * 3u)) & 63u;
    y.c = (x.c + (k & 7u)) & 7u;
    return y;
}

int main(void) {
    Bits in = {0u, 0u, 0u};
    in.a = 63u;
    in.b = 127u;
    in.c = 15u;

    Bits out = bounce(in, 9u);
    Bits copy = out;
    unsigned mix = copy.a + copy.b * 100u + copy.c * 10000u;

    printf("%u %u %u %u %u %u %u\n",
           in.a, in.b, in.c,
           out.a, out.b, out.c,
           mix);
    return 0;
}
