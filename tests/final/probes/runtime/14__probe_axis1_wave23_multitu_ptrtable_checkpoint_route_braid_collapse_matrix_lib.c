const unsigned axis1_wave23_collapse_weights[68] = {
    23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u, 67u,
    71u, 75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u, 107u, 111u, 115u,
    119u, 123u, 127u, 131u, 135u, 139u, 143u, 147u, 151u, 155u, 159u, 163u,
    167u, 171u, 175u, 179u, 183u, 187u, 191u, 195u, 199u, 203u, 207u, 211u,
    215u, 219u, 223u, 227u, 231u, 235u, 239u, 243u, 247u, 251u, 255u, 259u,
    263u, 267u, 271u, 275u, 279u, 283u, 287u, 291u,
};

const int axis1_wave23_collapse_signed_offsets[34] = {
    -19, 14, -16, 21, -13, 24, -18, 15, -11, 27, -17, 16, -14, 29, -12, 31,
    -15, 23, -10, 33, -20, 18, -9, 35, -8, 37, -7, 39, -6, 41, -5, 43, -4, 45,
};

const unsigned axis1_wave23_collapse_unsigned_offsets[34] = {
    19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u, 67u, 71u, 75u, 79u,
    83u, 87u, 91u, 95u, 99u, 103u, 107u, 111u, 115u, 119u, 123u, 127u, 131u, 135u, 139u, 143u,
    147u, 151u,
};

const unsigned* axis1_wave23_collapse_windows[34] = {
    axis1_wave23_collapse_weights + 0, axis1_wave23_collapse_weights + 2,
    axis1_wave23_collapse_weights + 4, axis1_wave23_collapse_weights + 6,
    axis1_wave23_collapse_weights + 8, axis1_wave23_collapse_weights + 10,
    axis1_wave23_collapse_weights + 12, axis1_wave23_collapse_weights + 14,
    axis1_wave23_collapse_weights + 16, axis1_wave23_collapse_weights + 18,
    axis1_wave23_collapse_weights + 20, axis1_wave23_collapse_weights + 22,
    axis1_wave23_collapse_weights + 24, axis1_wave23_collapse_weights + 26,
    axis1_wave23_collapse_weights + 28, axis1_wave23_collapse_weights + 30,
    axis1_wave23_collapse_weights + 32, axis1_wave23_collapse_weights + 34,
    axis1_wave23_collapse_weights + 1, axis1_wave23_collapse_weights + 3,
    axis1_wave23_collapse_weights + 5, axis1_wave23_collapse_weights + 7,
    axis1_wave23_collapse_weights + 9, axis1_wave23_collapse_weights + 11,
    axis1_wave23_collapse_weights + 13, axis1_wave23_collapse_weights + 15,
    axis1_wave23_collapse_weights + 17, axis1_wave23_collapse_weights + 19,
    axis1_wave23_collapse_weights + 21, axis1_wave23_collapse_weights + 23,
    axis1_wave23_collapse_weights + 25, axis1_wave23_collapse_weights + 27,
    axis1_wave23_collapse_weights + 29, axis1_wave23_collapse_weights + 31,
};

const unsigned** axis1_wave23_collapse_routes[16] = {
    axis1_wave23_collapse_windows + 0, axis1_wave23_collapse_windows + 2,
    axis1_wave23_collapse_windows + 4, axis1_wave23_collapse_windows + 6,
    axis1_wave23_collapse_windows + 8, axis1_wave23_collapse_windows + 10,
    axis1_wave23_collapse_windows + 12, axis1_wave23_collapse_windows + 14,
    axis1_wave23_collapse_windows + 16, axis1_wave23_collapse_windows + 18,
    axis1_wave23_collapse_windows + 20, axis1_wave23_collapse_windows + 22,
    axis1_wave23_collapse_windows + 24, axis1_wave23_collapse_windows + 26,
    axis1_wave23_collapse_windows + 28, axis1_wave23_collapse_windows + 30,
};

const unsigned*** axis1_wave23_collapse_plans[4] = {
    axis1_wave23_collapse_routes + 0,
    axis1_wave23_collapse_routes + 4,
    axis1_wave23_collapse_routes + 8,
    axis1_wave23_collapse_routes + 12,
};
