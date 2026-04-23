static unsigned axis1_wave7_owner_mix_impl(unsigned value, unsigned weight) {
    return (value * 31u) ^ (weight + 0x4Fu);
}

const unsigned axis1_wave7_owner_weights[8] = {2u, 6u, 10u, 14u, 18u, 22u, 26u, 30u};
const unsigned* axis1_wave7_owner_windows[3] = {
    axis1_wave7_owner_weights + 0,
    axis1_wave7_owner_weights + 2,
    axis1_wave7_owner_weights + 4,
};
const unsigned* const* axis1_wave7_owner_routes[3] = {
    axis1_wave7_owner_windows + 1,
    axis1_wave7_owner_windows + 2,
    axis1_wave7_owner_windows + 0,
};
unsigned (*axis1_wave7_owner_mix)(unsigned, unsigned) = axis1_wave7_owner_mix_impl;
