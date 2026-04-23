static unsigned axis1_wave14_route_mix_impl(unsigned value, unsigned weight) {
    return ((value ^ (weight + 0x77u)) * 31u) + (value >> 2);
}

const unsigned axis1_wave14_route_weights[26] = {
    8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u,
    60u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u, 108u,
};

const int axis1_wave14_route_signed_offsets[12] = {-7, 3, -4, 8, -2, 9, -6, 5, -1, 10, -5, 11};
const unsigned axis1_wave14_route_unsigned_offsets[12] = {4u, 8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u};

const unsigned* axis1_wave14_route_windows[13] = {
    axis1_wave14_route_weights + 0,
    axis1_wave14_route_weights + 2,
    axis1_wave14_route_weights + 4,
    axis1_wave14_route_weights + 6,
    axis1_wave14_route_weights + 8,
    axis1_wave14_route_weights + 10,
    axis1_wave14_route_weights + 12,
    axis1_wave14_route_weights + 14,
    axis1_wave14_route_weights + 16,
    axis1_wave14_route_weights + 18,
    axis1_wave14_route_weights + 20,
    axis1_wave14_route_weights + 22,
    axis1_wave14_route_weights + 24,
};

const unsigned** axis1_wave14_route_routes[8] = {
    axis1_wave14_route_windows + 0,
    axis1_wave14_route_windows + 2,
    axis1_wave14_route_windows + 4,
    axis1_wave14_route_windows + 6,
    axis1_wave14_route_windows + 1,
    axis1_wave14_route_windows + 3,
    axis1_wave14_route_windows + 5,
    axis1_wave14_route_windows + 7,
};

const unsigned*** axis1_wave14_route_checkpoints[6] = {
    axis1_wave14_route_routes + 0,
    axis1_wave14_route_routes + 2,
    axis1_wave14_route_routes + 4,
    axis1_wave14_route_routes + 1,
    axis1_wave14_route_routes + 3,
    axis1_wave14_route_routes + 5,
};

unsigned (*axis1_wave14_route_mix)(unsigned, unsigned) = axis1_wave14_route_mix_impl;
