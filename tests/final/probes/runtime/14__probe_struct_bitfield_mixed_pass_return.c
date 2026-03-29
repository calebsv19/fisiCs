#include <stdio.h>

typedef struct Packet {
    unsigned lo : 5;
    unsigned mid : 7;
    unsigned hi : 4;
} Packet;

static Packet mutate(Packet p, unsigned step) {
    Packet out = p;
    out.lo = (p.lo + step) & 31u;
    out.mid = (p.mid * 3u + step) & 127u;
    out.hi = (p.hi + step * 5u) & 15u;
    return out;
}

int main(void) {
    Packet in = {0u, 0u, 0u};
    in.lo = 63u;
    in.mid = 255u;
    in.hi = 31u;

    Packet a = mutate(in, 9u);
    Packet b = mutate(a, 2u);
    unsigned mix = a.lo
                 + a.mid * 100u
                 + a.hi * 10000u
                 + b.lo * 1000000u
                 + b.mid * 10000000u
                 + b.hi * 100000000u;

    printf("%u %u %u %u %u %u %u\n", a.lo, a.mid, a.hi, b.lo, b.mid, b.hi, mix);
    return 0;
}
