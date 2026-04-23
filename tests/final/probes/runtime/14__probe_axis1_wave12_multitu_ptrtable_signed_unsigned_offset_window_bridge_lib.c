const unsigned axis1_wave12_bridge_weights[26] = {
    3u, 7u, 11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u,
    55u, 59u, 63u, 67u, 71u, 75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u,
};

const int axis1_wave12_bridge_signed_offsets[12] = {-7, 4, -3, 9, -5, 2, -8, 6, -1, 10, -4, 5};
const unsigned axis1_wave12_bridge_unsigned_offsets[12] = {1u, 4u, 7u, 10u, 13u, 16u, 19u, 22u, 25u, 28u, 31u, 34u};

const unsigned* axis1_wave12_bridge_windows[13] = {
    axis1_wave12_bridge_weights + 0,
    axis1_wave12_bridge_weights + 2,
    axis1_wave12_bridge_weights + 4,
    axis1_wave12_bridge_weights + 6,
    axis1_wave12_bridge_weights + 8,
    axis1_wave12_bridge_weights + 10,
    axis1_wave12_bridge_weights + 12,
    axis1_wave12_bridge_weights + 14,
    axis1_wave12_bridge_weights + 16,
    axis1_wave12_bridge_weights + 18,
    axis1_wave12_bridge_weights + 20,
    axis1_wave12_bridge_weights + 22,
    axis1_wave12_bridge_weights + 24,
};

const unsigned** axis1_wave12_bridge_checkpoints[8] = {
    axis1_wave12_bridge_windows + 0,
    axis1_wave12_bridge_windows + 2,
    axis1_wave12_bridge_windows + 4,
    axis1_wave12_bridge_windows + 6,
    axis1_wave12_bridge_windows + 1,
    axis1_wave12_bridge_windows + 3,
    axis1_wave12_bridge_windows + 5,
    axis1_wave12_bridge_windows + 7,
};

const unsigned*** axis1_wave12_bridge_graph[4] = {
    axis1_wave12_bridge_checkpoints + 0,
    axis1_wave12_bridge_checkpoints + 2,
    axis1_wave12_bridge_checkpoints + 4,
    axis1_wave12_bridge_checkpoints + 1,
};
