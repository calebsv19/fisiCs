const unsigned axis1_wave13_blend_weights[28] = {
    4u, 8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u,
    60u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u, 108u, 112u,
};

const int axis1_wave13_signed_offsets[12] = {
    (3 * 5) - 9,
    (2 * 7) - 15,
    (4 * 3) - 18,
    (5 * 2) - 1,
    (6 * 2) - 17,
    (7 * 2) - 11,
    (8 * 2) - 21,
    (9 * 2) - 12,
    (10 * 2) - 19,
    (11 * 2) - 14,
    (12 * 2) - 23,
    (13 * 2) - 16,
};

const unsigned axis1_wave13_unsigned_offsets[12] = {
    (1u << 1), (2u << 1) + 1u, (3u << 1) + 2u, (4u << 1) + 3u,
    (5u << 1) + 4u, (6u << 1) + 5u, (7u << 1) + 6u, (8u << 1) + 7u,
    (9u << 1) + 8u, (10u << 1) + 9u, (11u << 1) + 10u, (12u << 1) + 11u,
};

const unsigned* axis1_wave13_blend_windows[14] = {
    axis1_wave13_blend_weights + 0,
    axis1_wave13_blend_weights + 2,
    axis1_wave13_blend_weights + 4,
    axis1_wave13_blend_weights + 6,
    axis1_wave13_blend_weights + 8,
    axis1_wave13_blend_weights + 10,
    axis1_wave13_blend_weights + 12,
    axis1_wave13_blend_weights + 14,
    axis1_wave13_blend_weights + 16,
    axis1_wave13_blend_weights + 18,
    axis1_wave13_blend_weights + 20,
    axis1_wave13_blend_weights + 22,
    axis1_wave13_blend_weights + 24,
    axis1_wave13_blend_weights + 26,
};

const unsigned** axis1_wave13_blend_routes[8] = {
    axis1_wave13_blend_windows + 0,
    axis1_wave13_blend_windows + 2,
    axis1_wave13_blend_windows + 4,
    axis1_wave13_blend_windows + 6,
    axis1_wave13_blend_windows + 1,
    axis1_wave13_blend_windows + 3,
    axis1_wave13_blend_windows + 5,
    axis1_wave13_blend_windows + 7,
};
