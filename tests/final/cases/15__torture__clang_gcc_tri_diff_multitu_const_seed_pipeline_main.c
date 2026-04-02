#include <stdio.h>

unsigned w25_seed_pick(unsigned idx);
unsigned w25_seed_fold(unsigned state, unsigned value, unsigned round);

int main(void) {
    unsigned state = 0x811c9dc5u;
    unsigned mirror = 0x1234abcdu;

    for (unsigned round = 0u; round < 144u; ++round) {
        unsigned a = w25_seed_pick(round + (state & 31u));
        unsigned b = w25_seed_pick(round * 9u + 7u);
        unsigned shift_a = (round & 3u) + 1u;
        unsigned shift_b = round & 1u;

        state = w25_seed_fold(state, a ^ (b >> shift_a), round);
        mirror = w25_seed_fold(mirror ^ state, b + (a << shift_b), round + 19u);
        mirror = (mirror << 3u) | (mirror >> 29u);
    }

    printf("%u %u\n", state, mirror);
    return 0;
}
