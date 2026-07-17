#include "15__probe_abi_wave130_units_callback_aggregate_preservation.h"
#include <stdio.h>

static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    if (shift == 0u) {
        return value;
    }
    return (value << shift) | (value >> (32u - shift));
}

int main(void) {
    struct Wave130Frame frames[3];
    Wave130CellFn callbacks[2];
    unsigned acc = 0x9e3779b9u;
    unsigned i;

    callbacks[0] = wave130_adjust_cell;
    callbacks[1] = wave130_fold_callback;

    frames[0] = wave130_seed_frame(8.5, 1.25, 210.0, 29u);
    frames[1] = wave130_seed_frame(5.75, 0.875, 175.0, 53u);
    frames[2] = wave130_seed_frame(11.0, 1.75, 260.0, 97u);

    for (i = 0u; i < 9u; ++i) {
        unsigned src = (i + frames[i % 3u].epoch) % 3u;
        unsigned dst = (src + 1u + (frames[src].cells[0].bits.named.route & 1u)) % 3u;
        frames[dst] = wave130_apply_frame(frames[src], callbacks[i & 1u], i + 7u);
        acc ^= wave130_frame_digest(frames[dst], rotl32(i * 257u + 41u, i + 4u));
        acc = rotl32(acc + frames[dst].epoch, 11u);
    }

    printf("%u %u %u\n",
           acc,
           wave130_frame_digest(frames[0], 0x85ebca6bu),
           wave130_frame_digest(frames[2], 0xc2b2ae35u));
    return 0;
}
