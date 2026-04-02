#include <stdio.h>

unsigned mt27_crc_pick(unsigned idx);
unsigned mt27_crc_fold(unsigned state, unsigned value, unsigned round);

int main(void) {
    unsigned state = 0x811c9dc5u;
    unsigned alt = 0x9e3779b9u;

    for (unsigned round = 0u; round < 192u; ++round) {
        unsigned a = mt27_crc_pick(round + (alt & 63u));
        unsigned b = mt27_crc_pick(round * 11u + 5u);
        unsigned sh = (round & 3u) + 1u;

        state = mt27_crc_fold(state, a ^ (b >> sh), round);
        alt ^= mt27_crc_fold(alt + round * 3u, b ^ (a << (round & 1u)), round + 17u);
        alt = (alt << 5u) | (alt >> 27u);
    }

    printf("%u %u\n", state, alt);
    return 0;
}
