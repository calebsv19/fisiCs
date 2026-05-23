const unsigned axis1_wave17_replay_weights[46] = {
    11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u,
    59u, 63u, 67u, 71u, 75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u,
    107u, 111u, 115u, 119u, 123u, 127u, 131u, 135u, 139u, 143u, 147u, 151u,
    155u, 159u, 163u, 167u, 171u, 175u, 179u, 183u, 187u, 191u,
};

const unsigned axis1_wave17_replay_lane_masks[12] = {
    2u, 5u, 7u, 9u, 3u, 11u, 13u, 6u, 10u, 14u, 12u, 15u,
};

const int axis1_wave17_replay_signed_offsets[20] = {
    -12, 7, -9, 13, -6, 15, -10, 8, -4, 17,
    -11, 9, -7, 18, -5, 20, -8, 16, -3, 21,
};

const unsigned axis1_wave17_replay_unsigned_offsets[20] = {
    8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u,
    48u, 52u, 56u, 60u, 64u, 68u, 72u, 76u, 80u, 84u,
};

const unsigned* axis1_wave17_replay_windows[23] = {
    axis1_wave17_replay_weights + 0,
    axis1_wave17_replay_weights + 2,
    axis1_wave17_replay_weights + 4,
    axis1_wave17_replay_weights + 6,
    axis1_wave17_replay_weights + 8,
    axis1_wave17_replay_weights + 10,
    axis1_wave17_replay_weights + 12,
    axis1_wave17_replay_weights + 14,
    axis1_wave17_replay_weights + 16,
    axis1_wave17_replay_weights + 18,
    axis1_wave17_replay_weights + 20,
    axis1_wave17_replay_weights + 22,
    axis1_wave17_replay_weights + 24,
    axis1_wave17_replay_weights + 26,
    axis1_wave17_replay_weights + 28,
    axis1_wave17_replay_weights + 30,
    axis1_wave17_replay_weights + 32,
    axis1_wave17_replay_weights + 34,
    axis1_wave17_replay_weights + 36,
    axis1_wave17_replay_weights + 38,
    axis1_wave17_replay_weights + 40,
    axis1_wave17_replay_weights + 42,
    axis1_wave17_replay_weights + 44,
};

const unsigned** axis1_wave17_replay_routes[12] = {
    axis1_wave17_replay_windows + 0,
    axis1_wave17_replay_windows + 2,
    axis1_wave17_replay_windows + 4,
    axis1_wave17_replay_windows + 6,
    axis1_wave17_replay_windows + 8,
    axis1_wave17_replay_windows + 10,
    axis1_wave17_replay_windows + 1,
    axis1_wave17_replay_windows + 3,
    axis1_wave17_replay_windows + 5,
    axis1_wave17_replay_windows + 7,
    axis1_wave17_replay_windows + 9,
    axis1_wave17_replay_windows + 11,
};

const unsigned*** axis1_wave17_replay_plans[3] = {
    axis1_wave17_replay_routes + 0,
    axis1_wave17_replay_routes + 4,
    axis1_wave17_replay_routes + 7,
};
