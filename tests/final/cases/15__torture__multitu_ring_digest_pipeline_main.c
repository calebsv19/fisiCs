#include <stdio.h>

unsigned mt13_ring_push(unsigned value);
unsigned mt13_ring_digest(void);

int main(void) {
    unsigned x = 0x13579bdu;
    unsigned total = 0u;

    for (unsigned i = 0; i < 64u; ++i) {
        x = x * 1664525u + 1013904223u;
        total ^= mt13_ring_push((x >> 3) ^ (i * 97u)) + i * 2654435761u;
    }

    printf("%u %u\n", total, mt13_ring_digest());
    return 0;
}
