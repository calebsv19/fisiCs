#include <stdio.h>

typedef struct {
    long double lane;
    unsigned tag;
} LdPacket;

typedef LdPacket (*LdMaker)(long double lane, unsigned tag);

static LdPacket make_a(long double lane, unsigned tag) {
    LdPacket p;
    p.lane = lane + (long double)(tag * 3u);
    p.tag = tag ^ 0x15u;
    return p;
}

static LdPacket make_b(long double lane, unsigned tag) {
    LdPacket p;
    p.lane = lane - (long double)(tag * 2u);
    p.tag = tag ^ 0x2Au;
    return p;
}

int main(void) {
    LdMaker table[2];
    unsigned i;
    unsigned digest = 61u;
    unsigned lane_sum = 0u;

    table[0] = make_a;
    table[1] = make_b;

    for (i = 0u; i < 6u; ++i) {
        LdPacket p = table[i & 1u]((long double)(1000u + i * 19u), 7u + i);
        unsigned lane_i = (unsigned)p.lane;
        lane_sum += lane_i;
        digest = digest * 173u + lane_i + p.tag * 11u;
    }

    printf("%u %u\n", lane_sum, digest);
    return 0;
}
