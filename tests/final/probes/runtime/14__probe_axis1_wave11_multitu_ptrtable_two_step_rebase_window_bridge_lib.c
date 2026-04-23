const unsigned axis1_wave11_bridge_weights[24] = {
    4u, 8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u,
    52u, 56u, 60u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u,
};

const unsigned* axis1_wave11_bridge_windows[12] = {
    axis1_wave11_bridge_weights + 0,
    axis1_wave11_bridge_weights + 2,
    axis1_wave11_bridge_weights + 4,
    axis1_wave11_bridge_weights + 6,
    axis1_wave11_bridge_weights + 8,
    axis1_wave11_bridge_weights + 10,
    axis1_wave11_bridge_weights + 12,
    axis1_wave11_bridge_weights + 14,
    axis1_wave11_bridge_weights + 16,
    axis1_wave11_bridge_weights + 18,
    axis1_wave11_bridge_weights + 20,
    axis1_wave11_bridge_weights + 22,
};

const unsigned** axis1_wave11_bridge_checkpoints[8] = {
    axis1_wave11_bridge_windows + 0,
    axis1_wave11_bridge_windows + 2,
    axis1_wave11_bridge_windows + 4,
    axis1_wave11_bridge_windows + 6,
    axis1_wave11_bridge_windows + 1,
    axis1_wave11_bridge_windows + 3,
    axis1_wave11_bridge_windows + 5,
    axis1_wave11_bridge_windows + 7,
};

const unsigned*** axis1_wave11_bridge_graph[4] = {
    axis1_wave11_bridge_checkpoints + 0,
    axis1_wave11_bridge_checkpoints + 2,
    axis1_wave11_bridge_checkpoints + 4,
    axis1_wave11_bridge_checkpoints + 1,
};
