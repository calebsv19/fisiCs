const unsigned axis1_wave12_checkpoint_weights[22] = {
    6u, 10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u,
    50u, 54u, 58u, 62u, 66u, 70u, 74u, 78u, 82u, 86u, 90u,
};

const int axis1_wave12_checkpoint_signed_offsets[10] = {-5, 3, -2, 7, -4, 1, -6, 8, -3, 5};
const unsigned axis1_wave12_checkpoint_unsigned_offsets[10] = {2u, 5u, 8u, 11u, 14u, 17u, 20u, 23u, 26u, 29u};

const unsigned* axis1_wave12_checkpoint_windows[11] = {
    axis1_wave12_checkpoint_weights + 0,
    axis1_wave12_checkpoint_weights + 2,
    axis1_wave12_checkpoint_weights + 4,
    axis1_wave12_checkpoint_weights + 6,
    axis1_wave12_checkpoint_weights + 8,
    axis1_wave12_checkpoint_weights + 10,
    axis1_wave12_checkpoint_weights + 12,
    axis1_wave12_checkpoint_weights + 14,
    axis1_wave12_checkpoint_weights + 16,
    axis1_wave12_checkpoint_weights + 18,
    axis1_wave12_checkpoint_weights + 20,
};

const unsigned** axis1_wave12_checkpoint_routes[6] = {
    axis1_wave12_checkpoint_windows + 0,
    axis1_wave12_checkpoint_windows + 2,
    axis1_wave12_checkpoint_windows + 4,
    axis1_wave12_checkpoint_windows + 1,
    axis1_wave12_checkpoint_windows + 3,
    axis1_wave12_checkpoint_windows + 5,
};

const unsigned*** axis1_wave12_checkpoint_plans[4] = {
    axis1_wave12_checkpoint_routes + 0,
    axis1_wave12_checkpoint_routes + 2,
    axis1_wave12_checkpoint_routes + 1,
    axis1_wave12_checkpoint_routes + 0,
};
