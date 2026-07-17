#include "15__probe_abi_wave131_units_nested_aggregate_callback_matrix.h"
#include <stdio.h>

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

int main(void) {
    struct Wave131Packet packets[2];
    Wave131SampleFn callbacks[2];
    unsigned acc = 0x7f4a7c15u;
    unsigned i;

    callbacks[0] = wave131_bias_sample;
    callbacks[1] = wave131_cross_sample;

    packets[0] = wave131_seed_packet(7.25, 1.375, 180.0, 31u);
    packets[1] = wave131_seed_packet(10.5, 0.9375, 240.0, 73u);

    for (i = 0u; i < 8u; ++i) {
        unsigned src = (packets[i & 1u].epoch + i) & 1u;
        unsigned dst = src ^ 1u;
        packets[dst] = wave131_apply_packet(packets[src], callbacks[i & 1u], i + 5u);
        acc ^= wave131_packet_digest(packets[dst], rotl32(i * 193u + 57u, i + 3u));
        acc = rotl32(acc + packets[dst].epoch, 9u);
    }

    printf("%u %u %u\n",
           acc,
           wave131_packet_digest(packets[0], 0x9e3779b9u),
           wave131_packet_digest(packets[1], 0x85ebca6bu));
    return 0;
}
