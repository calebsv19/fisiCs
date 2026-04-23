static unsigned axis1_wave6_mix_impl(unsigned value, unsigned weight) {
    return (value * 29u) ^ (weight + 0x9Du);
}

const unsigned axis1_wave6_owner_weights[4] = {3u, 11u, 19u, 27u};
const unsigned* axis1_wave6_owner_hops[2] = {
    axis1_wave6_owner_weights + 0,
    axis1_wave6_owner_weights + 2,
};
unsigned (*axis1_wave6_owner_mix_ptr)(unsigned, unsigned) = axis1_wave6_mix_impl;
