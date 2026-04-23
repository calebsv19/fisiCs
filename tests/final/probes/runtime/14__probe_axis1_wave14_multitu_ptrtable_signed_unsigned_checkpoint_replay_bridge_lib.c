const unsigned axis1_wave14_replay_weights[34] = {
    5u, 9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u,
    53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u, 85u, 89u, 93u, 97u,
    101u, 105u, 109u, 113u, 117u, 121u, 125u, 129u, 133u, 137u,
};

const int axis1_wave14_replay_signed_offsets[16] = {-9, 4, -6, 8, -3, 10, -7, 5, -2, 11, -8, 6, -4, 12, -5, 13};
const unsigned axis1_wave14_replay_unsigned_offsets[16] = {3u, 7u, 11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u};

const unsigned* axis1_wave14_replay_windows[17] = {
    axis1_wave14_replay_weights + 0,
    axis1_wave14_replay_weights + 2,
    axis1_wave14_replay_weights + 4,
    axis1_wave14_replay_weights + 6,
    axis1_wave14_replay_weights + 8,
    axis1_wave14_replay_weights + 10,
    axis1_wave14_replay_weights + 12,
    axis1_wave14_replay_weights + 14,
    axis1_wave14_replay_weights + 16,
    axis1_wave14_replay_weights + 18,
    axis1_wave14_replay_weights + 20,
    axis1_wave14_replay_weights + 22,
    axis1_wave14_replay_weights + 24,
    axis1_wave14_replay_weights + 26,
    axis1_wave14_replay_weights + 28,
    axis1_wave14_replay_weights + 30,
    axis1_wave14_replay_weights + 32,
};

const unsigned** axis1_wave14_replay_routes[10] = {
    axis1_wave14_replay_windows + 0,
    axis1_wave14_replay_windows + 2,
    axis1_wave14_replay_windows + 4,
    axis1_wave14_replay_windows + 6,
    axis1_wave14_replay_windows + 8,
    axis1_wave14_replay_windows + 1,
    axis1_wave14_replay_windows + 3,
    axis1_wave14_replay_windows + 5,
    axis1_wave14_replay_windows + 7,
    axis1_wave14_replay_windows + 9,
};
