#include "15__probe_abi_wave133_units_callback_envelope_chain.h"
#include <stdio.h>

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

int main(void) {
    struct Wave133ChainEnvelope envelope;
    Wave133SegmentFn callbacks[2];
    unsigned acc = 0x13198a2eu;
    unsigned i;

    callbacks[0] = wave133_chain_bias_segment;
    callbacks[1] = wave133_chain_cross_segment;
    envelope = wave133_chain_seed(8.875, 1.4375, 205.0, 825.0, 67u);

    for (i = 0u; i < 8u; ++i) {
        envelope = wave133_chain_apply(envelope, callbacks[i & 1u], i + 5u);
        acc ^= wave133_chain_digest(envelope, rotl32(0x9e3779b9u + i * 173u, i + 2u));
        acc = rotl32(acc + envelope.generation + i, 7u);
    }

    printf("%u %u %u\n",
           acc,
           wave133_chain_digest(envelope, 0x85ebca6bu),
           envelope.generation ^ envelope.envelope_footer.named.owner);
    return 0;
}
