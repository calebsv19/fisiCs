#include "15__probe_abi_wave129_units_tagged_payload_preservation.h"
#include <stdio.h>

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

int main(void) {
    struct Wave129AbiUnitPayload ring[3];
    Wave129AbiStep step = wave129_abi_mix;
    unsigned acc = 0x811c9dc5u;
    unsigned i;

    ring[0] = wave129_abi_seed(4.5, 0.75, 19u);
    ring[1] = wave129_abi_seed(7.25, 1.5, 41u);
    ring[2] = wave129_abi_seed(2.0, 0.625, 73u);

    for (i = 0u; i < 8u; ++i) {
        unsigned src = (i + ring[i % 3u].route) % 3u;
        unsigned dst = (src + 1u + (ring[src].bits.named.tag & 1u)) % 3u;
        ring[dst] = step(ring[src], i + 11u);
        acc ^= wave129_abi_fold(ring[dst], rotl32(i * 167u + 23u, i + 3u));
        acc = rotl32(acc + ring[dst].bits.named.mask, 9u);
    }

    printf("%u %u %u\n",
           acc,
           wave129_abi_fold(ring[0], 0x9e3779b9u),
           wave129_abi_fold(ring[2], 0x85ebca6bu));
    return 0;
}
