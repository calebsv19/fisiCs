#include <stdio.h>

unsigned probe15_crc_pick(unsigned idx);
unsigned probe15_crc_fold(unsigned state, unsigned value, unsigned round);

int main(void) {
    unsigned state = 0x811c9dc5u;
    unsigned alt = 0x12345678u;

    for (unsigned round = 0; round < 128u; ++round) {
        unsigned a = probe15_crc_pick(round);
        unsigned b = probe15_crc_pick(round * 7u + 3u);
        state = probe15_crc_fold(state, a ^ (b << 1u), round);
        alt ^= probe15_crc_fold(alt + round, b ^ (a >> 1u), round + 11u);
        alt = (alt << 3u) | (alt >> 29u);
    }

    printf("%u %u\n", state, alt);
    return 0;
}
