static unsigned axis1_wave10_owner_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x79u)) * 21u) + (value >> 2);
}

const unsigned axis1_wave10_owner_weights[12] = {
    5u, 9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u,
};
const unsigned* axis1_wave10_owner_windows[6] = {
    axis1_wave10_owner_weights + 0,
    axis1_wave10_owner_weights + 2,
    axis1_wave10_owner_weights + 4,
    axis1_wave10_owner_weights + 6,
    axis1_wave10_owner_weights + 8,
    axis1_wave10_owner_weights + 10,
};
const unsigned** axis1_wave10_owner_routes[5] = {
    axis1_wave10_owner_windows + 1,
    axis1_wave10_owner_windows + 3,
    axis1_wave10_owner_windows + 0,
    axis1_wave10_owner_windows + 4,
    axis1_wave10_owner_windows + 2,
};
unsigned (*axis1_wave10_owner_mix)(unsigned, unsigned) = axis1_wave10_owner_mix_impl;
