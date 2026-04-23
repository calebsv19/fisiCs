static unsigned axis1_wave15_route_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x83u)) * 37u) + (value >> 3);
}

const unsigned axis1_wave15_route_weights[28] = {
    9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u,
    65u, 69u, 73u, 77u, 81u, 85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u,
};

const int axis1_wave15_route_signed_offsets[14] = {
    -8, 4, -5, 9, -3, 10, -7, 6, -2, 11, -6, 12, -4, 13,
};

const unsigned axis1_wave15_route_unsigned_offsets[14] = {
    5u, 9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u,
};

const unsigned* axis1_wave15_route_windows[14] = {
    axis1_wave15_route_weights + 0,
    axis1_wave15_route_weights + 2,
    axis1_wave15_route_weights + 4,
    axis1_wave15_route_weights + 6,
    axis1_wave15_route_weights + 8,
    axis1_wave15_route_weights + 10,
    axis1_wave15_route_weights + 12,
    axis1_wave15_route_weights + 14,
    axis1_wave15_route_weights + 16,
    axis1_wave15_route_weights + 18,
    axis1_wave15_route_weights + 20,
    axis1_wave15_route_weights + 22,
    axis1_wave15_route_weights + 24,
    axis1_wave15_route_weights + 26,
};

const unsigned** axis1_wave15_route_routes[10] = {
    axis1_wave15_route_windows + 0,
    axis1_wave15_route_windows + 2,
    axis1_wave15_route_windows + 4,
    axis1_wave15_route_windows + 6,
    axis1_wave15_route_windows + 8,
    axis1_wave15_route_windows + 1,
    axis1_wave15_route_windows + 3,
    axis1_wave15_route_windows + 5,
    axis1_wave15_route_windows + 7,
    axis1_wave15_route_windows + 9,
};

const unsigned*** axis1_wave15_route_plans[3] = {
    axis1_wave15_route_routes + 0,
    axis1_wave15_route_routes + 4,
    axis1_wave15_route_routes + 2,
};

unsigned (*axis1_wave15_route_mix)(unsigned, unsigned) = axis1_wave15_route_mix_impl;
