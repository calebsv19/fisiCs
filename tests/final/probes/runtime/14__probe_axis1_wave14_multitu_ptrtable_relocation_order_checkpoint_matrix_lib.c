const unsigned axis1_wave14_weights[30] = {
    6u, 10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u,
    46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u, 78u, 82u,
    86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u, 122u,
};

const int axis1_wave14_signed_offsets[14] = {-8, 3, -5, 7, -2, 9, -6, 4, -1, 10, -7, 5, -3, 11};
const unsigned axis1_wave14_unsigned_offsets[14] = {2u, 5u, 8u, 11u, 14u, 17u, 20u, 23u, 26u, 29u, 32u, 35u, 38u, 41u};

const unsigned* axis1_wave14_windows[15] = {
    axis1_wave14_weights + 0,
    axis1_wave14_weights + 2,
    axis1_wave14_weights + 4,
    axis1_wave14_weights + 6,
    axis1_wave14_weights + 8,
    axis1_wave14_weights + 10,
    axis1_wave14_weights + 12,
    axis1_wave14_weights + 14,
    axis1_wave14_weights + 16,
    axis1_wave14_weights + 18,
    axis1_wave14_weights + 20,
    axis1_wave14_weights + 22,
    axis1_wave14_weights + 24,
    axis1_wave14_weights + 26,
    axis1_wave14_weights + 28,
};

const unsigned** axis1_wave14_routes[8] = {
    axis1_wave14_windows + 0,
    axis1_wave14_windows + 2,
    axis1_wave14_windows + 4,
    axis1_wave14_windows + 6,
    axis1_wave14_windows + 1,
    axis1_wave14_windows + 3,
    axis1_wave14_windows + 5,
    axis1_wave14_windows + 7,
};

const unsigned*** axis1_wave14_checkpoints[6] = {
    axis1_wave14_routes + 0,
    axis1_wave14_routes + 2,
    axis1_wave14_routes + 4,
    axis1_wave14_routes + 1,
    axis1_wave14_routes + 3,
    axis1_wave14_routes + 5,
};
