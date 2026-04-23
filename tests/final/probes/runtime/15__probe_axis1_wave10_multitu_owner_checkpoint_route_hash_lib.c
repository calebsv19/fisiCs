static unsigned axis1_wave10_route_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x55u)) * 27u) + (value >> 2);
}

const unsigned axis1_wave10_route_weights[16] = {
    6u, 10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u,
};
const unsigned* axis1_wave10_route_windows[8] = {
    axis1_wave10_route_weights + 0,
    axis1_wave10_route_weights + 2,
    axis1_wave10_route_weights + 4,
    axis1_wave10_route_weights + 6,
    axis1_wave10_route_weights + 8,
    axis1_wave10_route_weights + 10,
    axis1_wave10_route_weights + 12,
    axis1_wave10_route_weights + 14,
};
const unsigned** axis1_wave10_route_routes[6] = {
    axis1_wave10_route_windows + 1,
    axis1_wave10_route_windows + 3,
    axis1_wave10_route_windows + 5,
    axis1_wave10_route_windows + 0,
    axis1_wave10_route_windows + 2,
    axis1_wave10_route_windows + 4,
};
const unsigned*** axis1_wave10_route_plans[3] = {
    axis1_wave10_route_routes + 0,
    axis1_wave10_route_routes + 2,
    axis1_wave10_route_routes + 1,
};
unsigned (*axis1_wave10_route_mix)(unsigned, unsigned) = axis1_wave10_route_mix_impl;
