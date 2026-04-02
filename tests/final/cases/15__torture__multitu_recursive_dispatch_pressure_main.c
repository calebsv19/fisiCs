#include <stdio.h>

unsigned mt27_dispatch(unsigned depth, unsigned lhs, unsigned rhs);
unsigned mt27_terminal(void);

int main(void) {
    unsigned acc = 0u;
    unsigned seed = 0x31415926u;

    for (unsigned row = 0u; row < 10u; ++row) {
        for (unsigned col = 0u; col < 9u; ++col) {
            unsigned depth = 20u + ((row + col) % 12u);
            seed = seed * 1664525u + 1013904223u + row * 11u + col * 17u;
            acc ^= mt27_dispatch(depth, seed ^ row, (seed >> 1u) ^ (col * 29u));
            acc = (acc << 3u) | (acc >> 29u);
            acc += depth * 97u + row * 13u + col * 19u;
        }
    }

    printf("%u %u\n", acc, mt27_terminal());
    return 0;
}
