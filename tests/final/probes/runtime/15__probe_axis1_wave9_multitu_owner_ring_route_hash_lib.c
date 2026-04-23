static unsigned axis1_wave9_route_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x6Du)) * 23u) + (value >> 1);
}

const unsigned axis1_wave9_route_weights[14] = {
    4u, 8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u,
};
const unsigned* axis1_wave9_route_windows[7] = {
    axis1_wave9_route_weights + 0,
    axis1_wave9_route_weights + 2,
    axis1_wave9_route_weights + 4,
    axis1_wave9_route_weights + 6,
    axis1_wave9_route_weights + 8,
    axis1_wave9_route_weights + 10,
    axis1_wave9_route_weights + 12,
};
const unsigned** axis1_wave9_route_routes[5] = {
    axis1_wave9_route_windows + 1,
    axis1_wave9_route_windows + 3,
    axis1_wave9_route_windows + 5,
    axis1_wave9_route_windows + 0,
    axis1_wave9_route_windows + 2,
};
const unsigned*** axis1_wave9_route_plans[3] = {
    axis1_wave9_route_routes + 0,
    axis1_wave9_route_routes + 2,
    axis1_wave9_route_routes + 1,
};
unsigned (*axis1_wave9_route_mix)(unsigned, unsigned) = axis1_wave9_route_mix_impl;
