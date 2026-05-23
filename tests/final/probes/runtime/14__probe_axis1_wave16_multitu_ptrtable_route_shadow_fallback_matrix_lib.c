const unsigned axis1_wave16_shadow_weights[42] = {
    8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u,
    56u, 60u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u,
    104u, 108u, 112u, 116u, 120u, 124u, 128u, 132u, 136u, 140u, 144u, 148u,
    152u, 156u, 160u, 164u, 168u, 172u,
};

const unsigned axis1_wave16_shadow_lane_masks[12] = {
    1u, 3u, 7u, 2u, 6u, 5u, 4u, 9u, 11u, 13u, 10u, 15u,
};

const int axis1_wave16_shadow_signed_offsets[18] = {
    -9, 4, -7, 10, -5, 12, -8, 6, -3,
    14, -10, 7, -6, 15, -4, 16, -11, 8,
};

const unsigned axis1_wave16_shadow_unsigned_offsets[18] = {
    6u, 10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u,
    42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u,
};

const unsigned* axis1_wave16_shadow_windows[21] = {
    axis1_wave16_shadow_weights + 0,
    axis1_wave16_shadow_weights + 2,
    axis1_wave16_shadow_weights + 4,
    axis1_wave16_shadow_weights + 6,
    axis1_wave16_shadow_weights + 8,
    axis1_wave16_shadow_weights + 10,
    axis1_wave16_shadow_weights + 12,
    axis1_wave16_shadow_weights + 14,
    axis1_wave16_shadow_weights + 16,
    axis1_wave16_shadow_weights + 18,
    axis1_wave16_shadow_weights + 20,
    axis1_wave16_shadow_weights + 22,
    axis1_wave16_shadow_weights + 24,
    axis1_wave16_shadow_weights + 26,
    axis1_wave16_shadow_weights + 28,
    axis1_wave16_shadow_weights + 30,
    axis1_wave16_shadow_weights + 32,
    axis1_wave16_shadow_weights + 34,
    axis1_wave16_shadow_weights + 36,
    axis1_wave16_shadow_weights + 38,
    axis1_wave16_shadow_weights + 40,
};

const unsigned** axis1_wave16_shadow_routes[11] = {
    axis1_wave16_shadow_windows + 0,
    axis1_wave16_shadow_windows + 2,
    axis1_wave16_shadow_windows + 4,
    axis1_wave16_shadow_windows + 6,
    axis1_wave16_shadow_windows + 8,
    axis1_wave16_shadow_windows + 10,
    axis1_wave16_shadow_windows + 1,
    axis1_wave16_shadow_windows + 3,
    axis1_wave16_shadow_windows + 5,
    axis1_wave16_shadow_windows + 7,
    axis1_wave16_shadow_windows + 9,
};

const unsigned*** axis1_wave16_shadow_plans[3] = {
    axis1_wave16_shadow_routes + 0,
    axis1_wave16_shadow_routes + 3,
    axis1_wave16_shadow_routes + 6,
};
