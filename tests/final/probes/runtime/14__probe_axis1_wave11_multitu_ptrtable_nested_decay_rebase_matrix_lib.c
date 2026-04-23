const unsigned axis1_wave11_decay_weights[20] = {
    5u, 9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u,
    45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u,
};

const unsigned* axis1_wave11_decay_windows[10] = {
    axis1_wave11_decay_weights + 0,
    axis1_wave11_decay_weights + 2,
    axis1_wave11_decay_weights + 4,
    axis1_wave11_decay_weights + 6,
    axis1_wave11_decay_weights + 8,
    axis1_wave11_decay_weights + 10,
    axis1_wave11_decay_weights + 12,
    axis1_wave11_decay_weights + 14,
    axis1_wave11_decay_weights + 16,
    axis1_wave11_decay_weights + 18,
};

const unsigned** axis1_wave11_decay_routes[6] = {
    axis1_wave11_decay_windows + 1,
    axis1_wave11_decay_windows + 3,
    axis1_wave11_decay_windows + 5,
    axis1_wave11_decay_windows + 0,
    axis1_wave11_decay_windows + 2,
    axis1_wave11_decay_windows + 4,
};

const unsigned*** axis1_wave11_decay_plans[4] = {
    axis1_wave11_decay_routes + 0,
    axis1_wave11_decay_routes + 1,
    axis1_wave11_decay_routes + 2,
    axis1_wave11_decay_routes + 0,
};

const unsigned axis1_wave11_decay_bias[8] = {7u, 12u, 18u, 24u, 31u, 39u, 46u, 54u};
