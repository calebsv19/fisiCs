#include <stdio.h>

typedef struct Bits {
    unsigned lo : 4;
    unsigned mid : 6;
    unsigned hi : 5;
    unsigned flag : 1;
} Bits;

static Bits mutate(Bits x, unsigned step) {
    x.lo = x.lo + step;
    x.mid = x.mid + (step << 1);
    x.hi = x.hi ^ step;
    x.flag = (x.mid >> 2) & 1u;
    return x;
}

int main(void) {
    Bits a = {0, 0, 0, 0};
    a.lo = 0x1fu;
    a.mid = 0x7fu;
    a.hi = 0x3fu;
    a.flag = 3u;

    Bits b = mutate(a, 9u);
    Bits c = b;
    c.mid = c.mid + 70u;

    unsigned score = b.lo + b.mid * 100u + b.hi * 10000u + b.flag * 1000000u;

    printf("%u %u %u %u %u %u %u %u %u\n",
           b.lo, b.mid, b.hi, b.flag,
           c.lo, c.mid, c.hi, c.flag,
           score);
    return 0;
}
