#include "15__probe_abi_wave132_units_envelope_callback_handoff.h"
#include <stdio.h>

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

int main(void) {
    struct Wave132Envelope envelope;
    Wave132FrameFn callbacks[2];
    unsigned acc = 0x6a09e667u;
    unsigned i;

    callbacks[0] = wave132_bias_frame;
    callbacks[1] = wave132_shear_frame;
    envelope = wave132_seed_envelope(9.75, 1.625, 220.0, 875.0, 43u);

    for (i = 0u; i < 9u; ++i) {
        envelope = wave132_apply_envelope(envelope, callbacks[i & 1u], i + 4u);
        acc ^= wave132_envelope_digest(envelope, rotl32(0x9e3779b9u + i * 97u, i + 2u));
        acc = rotl32(acc + envelope.generation + i, 7u);
    }

    printf("%u %u %u\n",
           acc,
           wave132_envelope_digest(envelope, 0x85ebca6bu),
           envelope.generation ^ envelope.footer.named.owner);
    return 0;
}
