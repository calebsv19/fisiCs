#include <stdio.h>

unsigned w25_fn_step(unsigned row, unsigned col, unsigned seed);
unsigned w25_fn_state(void);

int main(void) {
    unsigned acc = 0u;
    unsigned seed = 25u;

    for (unsigned row = 0u; row < 8u; ++row) {
        for (unsigned col = 0u; col < 8u; ++col) {
            seed = (seed * 33u + row * 17u + col * 9u + 0x45u) % 65521u;
            acc ^= w25_fn_step(row, col, seed) + row * 13u + col * 7u;
            acc = (acc << 5u) | (acc >> 27u);
            acc += seed ^ (row * 29u + col * 31u);
        }
    }

    printf("%u %u\n", acc, w25_fn_state());
    return 0;
}
