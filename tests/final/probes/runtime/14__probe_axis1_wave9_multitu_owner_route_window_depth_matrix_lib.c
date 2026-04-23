static unsigned axis1_wave9_owner_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x63u)) * 17u) + (value >> 1);
}

const unsigned axis1_wave9_owner_weights[10] = {
    2u, 6u, 10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u,
};
const unsigned* axis1_wave9_owner_windows[5] = {
    axis1_wave9_owner_weights + 0,
    axis1_wave9_owner_weights + 2,
    axis1_wave9_owner_weights + 4,
    axis1_wave9_owner_weights + 6,
    axis1_wave9_owner_weights + 8,
};
const unsigned* const* axis1_wave9_owner_routes[4] = {
    axis1_wave9_owner_windows + 1,
    axis1_wave9_owner_windows + 3,
    axis1_wave9_owner_windows + 0,
    axis1_wave9_owner_windows + 2,
};
unsigned (*axis1_wave9_owner_mix)(unsigned, unsigned) = axis1_wave9_owner_mix_impl;
