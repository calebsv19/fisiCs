const unsigned axis1_wave15_weights[36] = {
    7u, 11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u,
    55u, 59u, 63u, 67u, 71u, 75u, 79u, 83u, 87u, 91u, 95u, 99u,
    103u, 107u, 111u, 115u, 119u, 123u, 127u, 131u, 135u, 139u, 143u, 147u,
};

const int axis1_wave15_signed_offsets[16] = {
    -10, 5, -7, 9, -4, 11, -8, 6, -3, 12, -9, 7, -5, 13, -6, 14,
};

const unsigned axis1_wave15_unsigned_offsets[16] = {
    3u, 7u, 11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u,
};

const unsigned* axis1_wave15_windows[18] = {
    axis1_wave15_weights + 0,
    axis1_wave15_weights + 2,
    axis1_wave15_weights + 4,
    axis1_wave15_weights + 6,
    axis1_wave15_weights + 8,
    axis1_wave15_weights + 10,
    axis1_wave15_weights + 12,
    axis1_wave15_weights + 14,
    axis1_wave15_weights + 16,
    axis1_wave15_weights + 18,
    axis1_wave15_weights + 20,
    axis1_wave15_weights + 22,
    axis1_wave15_weights + 24,
    axis1_wave15_weights + 26,
    axis1_wave15_weights + 28,
    axis1_wave15_weights + 30,
    axis1_wave15_weights + 32,
    axis1_wave15_weights + 34,
};

const unsigned** axis1_wave15_routes[10] = {
    axis1_wave15_windows + 0,
    axis1_wave15_windows + 2,
    axis1_wave15_windows + 4,
    axis1_wave15_windows + 6,
    axis1_wave15_windows + 8,
    axis1_wave15_windows + 1,
    axis1_wave15_windows + 3,
    axis1_wave15_windows + 5,
    axis1_wave15_windows + 7,
    axis1_wave15_windows + 9,
};

const unsigned*** axis1_wave15_plans[3] = {
    axis1_wave15_routes + 0,
    axis1_wave15_routes + 3,
    axis1_wave15_routes + 6,
};
