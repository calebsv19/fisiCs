#include <stdio.h>

unsigned mt15_reseed(unsigned seed);
unsigned mt15_emit(unsigned lane, unsigned input);
unsigned mt15_checksum(void);

int main(void) {
    unsigned acc = 0u;
    unsigned seed = 0x1234u;
    mt15_reseed(seed);

    for (unsigned round = 0; round < 40u; ++round) {
        seed = seed * 1103515245u + 12345u;
        if ((round % 13u) == 0u) {
            mt15_reseed(seed ^ (round * 97u));
        }
        acc ^= mt15_emit(round % 7u, seed >> 8u) + round * 2654435761u;
    }

    printf("%u %u\n", acc, mt15_checksum());
    return 0;
}
