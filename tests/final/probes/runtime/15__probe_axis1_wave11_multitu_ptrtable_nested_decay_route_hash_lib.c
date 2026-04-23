static unsigned axis1_wave11_route_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x43u)) * 19u) + (value >> 1);
}

const unsigned axis1_wave11_route_weights[18] = {
    7u, 11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u,
    43u, 47u, 51u, 55u, 59u, 63u, 67u, 71u, 75u,
};

const unsigned* axis1_wave11_route_windows[9] = {
    axis1_wave11_route_weights + 0,
    axis1_wave11_route_weights + 2,
    axis1_wave11_route_weights + 4,
    axis1_wave11_route_weights + 6,
    axis1_wave11_route_weights + 8,
    axis1_wave11_route_weights + 10,
    axis1_wave11_route_weights + 12,
    axis1_wave11_route_weights + 14,
    axis1_wave11_route_weights + 16,
};

const unsigned** axis1_wave11_route_routes[6] = {
    axis1_wave11_route_windows + 0,
    axis1_wave11_route_windows + 2,
    axis1_wave11_route_windows + 4,
    axis1_wave11_route_windows + 1,
    axis1_wave11_route_windows + 3,
    axis1_wave11_route_windows + 5,
};

const unsigned*** axis1_wave11_route_plans[3] = {
    axis1_wave11_route_routes + 0,
    axis1_wave11_route_routes + 2,
    axis1_wave11_route_routes + 1,
};

unsigned (*axis1_wave11_route_mix)(unsigned, unsigned) = axis1_wave11_route_mix_impl;
