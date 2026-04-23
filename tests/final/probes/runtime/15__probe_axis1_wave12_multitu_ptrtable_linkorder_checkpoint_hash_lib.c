static unsigned axis1_wave12_route_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x5Du)) * 23u) + (value >> 2);
}

const unsigned axis1_wave12_route_weights[20] = {
    8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u,
    48u, 52u, 56u, 60u, 64u, 68u, 72u, 76u, 80u, 84u,
};

const int axis1_wave12_route_signed_offsets[10] = {-6, 2, -1, 7, -4, 3, -5, 8, -2, 6};
const unsigned axis1_wave12_route_unsigned_offsets[10] = {3u, 6u, 9u, 12u, 15u, 18u, 21u, 24u, 27u, 30u};

const unsigned* axis1_wave12_route_windows[10] = {
    axis1_wave12_route_weights + 0,
    axis1_wave12_route_weights + 2,
    axis1_wave12_route_weights + 4,
    axis1_wave12_route_weights + 6,
    axis1_wave12_route_weights + 8,
    axis1_wave12_route_weights + 10,
    axis1_wave12_route_weights + 12,
    axis1_wave12_route_weights + 14,
    axis1_wave12_route_weights + 16,
    axis1_wave12_route_weights + 18,
};

const unsigned** axis1_wave12_route_routes[6] = {
    axis1_wave12_route_windows + 0,
    axis1_wave12_route_windows + 2,
    axis1_wave12_route_windows + 4,
    axis1_wave12_route_windows + 1,
    axis1_wave12_route_windows + 3,
    axis1_wave12_route_windows + 5,
};

const unsigned*** axis1_wave12_route_plans[3] = {
    axis1_wave12_route_routes + 0,
    axis1_wave12_route_routes + 2,
    axis1_wave12_route_routes + 1,
};

unsigned (*axis1_wave12_route_mix)(unsigned, unsigned) = axis1_wave12_route_mix_impl;
