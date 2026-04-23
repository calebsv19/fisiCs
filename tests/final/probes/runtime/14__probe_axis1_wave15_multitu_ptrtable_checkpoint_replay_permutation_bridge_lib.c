const unsigned axis1_wave15_replay_weights[38] = {
    6u, 10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u,
    58u, 62u, 66u, 70u, 74u, 78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u,
    110u, 114u, 118u, 122u, 126u, 130u, 134u, 138u, 142u, 146u, 150u, 154u,
};

const int axis1_wave15_replay_signed_offsets[18] = {
    -11, 6, -8, 10, -5, 12, -9, 7, -4, 13, -10, 8, -6, 14, -7, 15, -3, 16,
};

const unsigned axis1_wave15_replay_unsigned_offsets[18] = {
    4u, 8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u, 60u, 64u, 68u, 72u,
};

const unsigned* axis1_wave15_replay_windows[19] = {
    axis1_wave15_replay_weights + 0,
    axis1_wave15_replay_weights + 2,
    axis1_wave15_replay_weights + 4,
    axis1_wave15_replay_weights + 6,
    axis1_wave15_replay_weights + 8,
    axis1_wave15_replay_weights + 10,
    axis1_wave15_replay_weights + 12,
    axis1_wave15_replay_weights + 14,
    axis1_wave15_replay_weights + 16,
    axis1_wave15_replay_weights + 18,
    axis1_wave15_replay_weights + 20,
    axis1_wave15_replay_weights + 22,
    axis1_wave15_replay_weights + 24,
    axis1_wave15_replay_weights + 26,
    axis1_wave15_replay_weights + 28,
    axis1_wave15_replay_weights + 30,
    axis1_wave15_replay_weights + 32,
    axis1_wave15_replay_weights + 34,
    axis1_wave15_replay_weights + 36,
};

const unsigned** axis1_wave15_replay_routes[10] = {
    axis1_wave15_replay_windows + 0,
    axis1_wave15_replay_windows + 2,
    axis1_wave15_replay_windows + 4,
    axis1_wave15_replay_windows + 6,
    axis1_wave15_replay_windows + 8,
    axis1_wave15_replay_windows + 1,
    axis1_wave15_replay_windows + 3,
    axis1_wave15_replay_windows + 5,
    axis1_wave15_replay_windows + 7,
    axis1_wave15_replay_windows + 9,
};

const unsigned*** axis1_wave15_replay_plans[3] = {
    axis1_wave15_replay_routes + 0,
    axis1_wave15_replay_routes + 4,
    axis1_wave15_replay_routes + 2,
};
