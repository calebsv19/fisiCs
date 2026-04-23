#include <stdio.h>

#include "15__bridge_inline_reset_generation_shared.h"

void bridge15_init(unsigned seed);
unsigned bridge15_step(unsigned lane, unsigned payload);
unsigned bridge15_digest(void);

int main(void) {
    unsigned i;
    unsigned acc = 0u;

    bridge15_init(17u);

    for (i = 0u; i < 32u; ++i) {
        unsigned lane = (i * 5u + 1u) % 3u;
        unsigned payload = i * 29u + 7u;
        acc ^= bridge15_step(lane, payload) + bridge15_scramble(i + 3u);
    }

    printf("%u %u\n", acc, bridge15_digest());
    return 0;
}
