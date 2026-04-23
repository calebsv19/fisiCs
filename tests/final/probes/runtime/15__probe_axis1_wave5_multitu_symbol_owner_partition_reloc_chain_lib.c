static unsigned axis1_wave5_mix(unsigned value) {
    return (value * 17u) + 13u;
}

unsigned (*axis1_wave5_mix_ptr)(unsigned) = axis1_wave5_mix;
const unsigned axis1_wave5_owner_weights[3] = {2u, 7u, 19u};
