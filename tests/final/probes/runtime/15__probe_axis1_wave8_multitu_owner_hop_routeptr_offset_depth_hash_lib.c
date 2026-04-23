static unsigned axis1_wave8_hop_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x71u)) * 19u) + (value >> 1);
}

const unsigned axis1_wave8_hop_weights[12] = {
    3u, 7u, 11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u,
};
const unsigned* axis1_wave8_hop_windows[6] = {
    axis1_wave8_hop_weights + 0,
    axis1_wave8_hop_weights + 2,
    axis1_wave8_hop_weights + 4,
    axis1_wave8_hop_weights + 6,
    axis1_wave8_hop_weights + 8,
    axis1_wave8_hop_weights + 10,
};
const unsigned** axis1_wave8_hop_routes[5] = {
    axis1_wave8_hop_windows + 1,
    axis1_wave8_hop_windows + 3,
    axis1_wave8_hop_windows + 0,
    axis1_wave8_hop_windows + 4,
    axis1_wave8_hop_windows + 2,
};
const unsigned*** axis1_wave8_owner_plans[3] = {
    axis1_wave8_hop_routes + 0,
    axis1_wave8_hop_routes + 2,
    axis1_wave8_hop_routes + 1,
};
unsigned (*axis1_wave8_hop_mix)(unsigned, unsigned) = axis1_wave8_hop_mix_impl;
