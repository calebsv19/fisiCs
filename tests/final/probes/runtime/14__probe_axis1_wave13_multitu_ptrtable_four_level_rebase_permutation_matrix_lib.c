const unsigned axis1_wave13_weights[32] = {
    5u, 9u, 13u, 17u, 21u, 25u, 29u, 33u,
    37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u,
    69u, 73u, 77u, 81u, 85u, 89u, 93u, 97u,
    101u, 105u, 109u, 113u, 117u, 121u, 125u, 129u,
};

const unsigned* axis1_wave13_windows[16] = {
    axis1_wave13_weights + 0,
    axis1_wave13_weights + 2,
    axis1_wave13_weights + 4,
    axis1_wave13_weights + 6,
    axis1_wave13_weights + 8,
    axis1_wave13_weights + 10,
    axis1_wave13_weights + 12,
    axis1_wave13_weights + 14,
    axis1_wave13_weights + 16,
    axis1_wave13_weights + 18,
    axis1_wave13_weights + 20,
    axis1_wave13_weights + 22,
    axis1_wave13_weights + 24,
    axis1_wave13_weights + 26,
    axis1_wave13_weights + 28,
    axis1_wave13_weights + 30,
};

const unsigned** axis1_wave13_routes[8] = {
    axis1_wave13_windows + 0,
    axis1_wave13_windows + 2,
    axis1_wave13_windows + 4,
    axis1_wave13_windows + 6,
    axis1_wave13_windows + 1,
    axis1_wave13_windows + 3,
    axis1_wave13_windows + 5,
    axis1_wave13_windows + 7,
};

const unsigned*** axis1_wave13_checkpoints[6] = {
    axis1_wave13_routes + 0,
    axis1_wave13_routes + 2,
    axis1_wave13_routes + 4,
    axis1_wave13_routes + 1,
    axis1_wave13_routes + 3,
    axis1_wave13_routes + 5,
};

const unsigned**** axis1_wave13_plans[3] = {
    axis1_wave13_checkpoints + 0,
    axis1_wave13_checkpoints + 2,
    axis1_wave13_checkpoints + 4,
};
