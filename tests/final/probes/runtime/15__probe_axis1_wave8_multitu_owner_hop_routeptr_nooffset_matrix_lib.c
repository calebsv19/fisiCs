static unsigned axis1_wave8_nooffset_mix_impl(unsigned value, unsigned weight) {
    return (value * 13u) + (weight ^ 0x45u);
}

const unsigned axis1_wave8_nooffset_weights[8] = {
    5u, 9u, 13u, 17u, 21u, 25u, 29u, 33u,
};
const unsigned* axis1_wave8_nooffset_windows[4] = {
    axis1_wave8_nooffset_weights + 0,
    axis1_wave8_nooffset_weights + 2,
    axis1_wave8_nooffset_weights + 4,
    axis1_wave8_nooffset_weights + 6,
};
const unsigned** axis1_wave8_nooffset_routes[3] = {
    axis1_wave8_nooffset_windows,
    axis1_wave8_nooffset_windows,
    axis1_wave8_nooffset_windows,
};
const unsigned*** axis1_wave8_nooffset_plans[2] = {
    axis1_wave8_nooffset_routes,
    axis1_wave8_nooffset_routes,
};
unsigned (*axis1_wave8_nooffset_mix)(unsigned, unsigned) = axis1_wave8_nooffset_mix_impl;
