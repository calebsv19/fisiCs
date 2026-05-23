const unsigned axis1_wave16_epoch_weights[40] = {
    9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u,
    49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u, 85u,
    89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u, 125u,
    129u, 133u, 137u, 141u, 145u, 149u, 153u, 157u, 161u, 165u,
};

const int axis1_wave16_epoch_signed_offsets[20] = {
    -12, 5, -9, 11, -6, 13, -10, 7, -4, 15,
    -11, 8, -7, 16, -5, 18, -8, 14, -3, 19,
};

const unsigned axis1_wave16_epoch_unsigned_offsets[20] = {
    5u, 9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u,
    45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u,
};

const unsigned* axis1_wave16_epoch_windows[20] = {
    axis1_wave16_epoch_weights + 0,
    axis1_wave16_epoch_weights + 2,
    axis1_wave16_epoch_weights + 4,
    axis1_wave16_epoch_weights + 6,
    axis1_wave16_epoch_weights + 8,
    axis1_wave16_epoch_weights + 10,
    axis1_wave16_epoch_weights + 12,
    axis1_wave16_epoch_weights + 14,
    axis1_wave16_epoch_weights + 16,
    axis1_wave16_epoch_weights + 18,
    axis1_wave16_epoch_weights + 20,
    axis1_wave16_epoch_weights + 22,
    axis1_wave16_epoch_weights + 24,
    axis1_wave16_epoch_weights + 26,
    axis1_wave16_epoch_weights + 28,
    axis1_wave16_epoch_weights + 30,
    axis1_wave16_epoch_weights + 32,
    axis1_wave16_epoch_weights + 34,
    axis1_wave16_epoch_weights + 36,
    axis1_wave16_epoch_weights + 38,
};

const unsigned** axis1_wave16_epoch_routes[10] = {
    axis1_wave16_epoch_windows + 0,
    axis1_wave16_epoch_windows + 2,
    axis1_wave16_epoch_windows + 4,
    axis1_wave16_epoch_windows + 6,
    axis1_wave16_epoch_windows + 8,
    axis1_wave16_epoch_windows + 1,
    axis1_wave16_epoch_windows + 3,
    axis1_wave16_epoch_windows + 5,
    axis1_wave16_epoch_windows + 7,
    axis1_wave16_epoch_windows + 9,
};

const unsigned*** axis1_wave16_epoch_plans[4] = {
    axis1_wave16_epoch_routes + 0,
    axis1_wave16_epoch_routes + 2,
    axis1_wave16_epoch_routes + 4,
    axis1_wave16_epoch_routes + 6,
};
