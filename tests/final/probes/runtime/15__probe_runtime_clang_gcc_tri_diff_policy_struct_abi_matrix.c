#include <stdio.h>

typedef struct {
    unsigned short tag;
    short delta;
    unsigned char flags;
    unsigned char mode;
    unsigned value;
} Packet;

static Packet mutate(Packet p, unsigned k) {
    int d = (int)p.delta + (int)(k % 23u) - 11;
    unsigned v = p.value * 33u + k * 17u + (unsigned)(unsigned short)(d + 257);
    unsigned char f = (unsigned char)((unsigned)p.flags ^ (k * 3u));
    p.delta = (short)d;
    p.flags = (unsigned char)(f + (unsigned char)p.tag);
    p.mode = (unsigned char)((p.mode + (unsigned char)(k & 7u)) & 7u);
    p.tag = (unsigned short)(p.tag + (unsigned short)(p.flags + k));
    p.value = v ^ ((unsigned)p.mode << 13u);
    return p;
}

static unsigned fold8(
    unsigned a0, unsigned a1, unsigned a2, unsigned a3,
    unsigned a4, unsigned a5, unsigned a6, unsigned a7
) {
    unsigned acc = 0x7f4a7c15u;
    acc ^= a0 + a4 * 3u;
    acc ^= a1 + a5 * 5u;
    acc ^= a2 + a6 * 7u;
    acc ^= a3 + a7 * 11u;
    acc = (acc << 9u) | (acc >> 23u);
    return acc ^ (acc >> 13u);
}

static unsigned reduce(Packet a, Packet b, Packet c, Packet d, unsigned k0, unsigned k1) {
    unsigned x0 = (unsigned)a.tag + (unsigned)(unsigned short)(a.delta + 256);
    unsigned x1 = (unsigned)b.flags + b.value;
    unsigned x2 = (unsigned)c.tag + (unsigned)c.mode * 17u;
    unsigned x3 = d.value ^ (unsigned)d.flags;
    unsigned x4 = k0 + (unsigned)a.mode + (unsigned)b.mode;
    unsigned x5 = k1 + (unsigned)c.mode + (unsigned)d.mode;
    unsigned x6 = (unsigned)(unsigned short)(b.delta + 512) + (unsigned)(unsigned short)(d.delta + 1024);
    unsigned x7 = a.value ^ b.value ^ c.value ^ d.value;
    return fold8(x0, x1, x2, x3, x4, x5, x6, x7);
}

int main(void) {
    Packet p0 = {3u, -7, 5u, 1u, 101u};
    Packet p1 = {11u, 9, 2u, 3u, 303u};
    Packet p2 = {19u, -3, 7u, 2u, 707u};
    Packet p3 = {23u, 4, 1u, 0u, 919u};
    unsigned r0;
    unsigned r1;
    unsigned r2;

    p0 = mutate(p0, 5u);
    p1 = mutate(p1, 9u);
    p2 = mutate(p2, 13u);
    p3 = mutate(p3, 17u);

    r0 = reduce(p0, p1, p2, p3, 29u, 31u);
    p0 = mutate(p0, 21u);
    p2 = mutate(p2, 25u);
    r1 = reduce(p3, p2, p1, p0, 37u, 41u);
    r2 = fold8(r0, r1, p0.value, p1.value, p2.value, p3.value, 43u, 47u);

    printf("%u %u\n", r2, r0 ^ (r1 + r2 * 3u));
    return 0;
}
