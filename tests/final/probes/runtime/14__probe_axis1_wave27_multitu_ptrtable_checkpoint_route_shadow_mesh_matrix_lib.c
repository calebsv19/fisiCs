const unsigned axis1_wave27_shadow_weights[84] = {
    35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u, 67u, 71u, 75u, 79u,
    83u, 87u, 91u, 95u, 99u, 103u, 107u, 111u, 115u, 119u, 123u, 127u,
    131u, 135u, 139u, 143u, 147u, 151u, 155u, 159u, 163u, 167u, 171u, 175u,
    179u, 183u, 187u, 191u, 195u, 199u, 203u, 207u, 211u, 215u, 219u, 223u,
    227u, 231u, 235u, 239u, 243u, 247u, 251u, 255u, 259u, 263u, 267u, 271u,
    275u, 279u, 283u, 287u, 291u, 295u, 299u, 303u, 307u, 311u, 315u, 319u,
    323u, 327u, 331u, 335u, 339u, 343u, 347u, 351u, 355u, 359u, 363u, 367u,
};

const int axis1_wave27_shadow_signed_offsets[42] = {
    -23, 18, -20, 25, -17, 28, -22, 19, -15, 31, -21, 20, -18, 33, -16, 35,
    -19, 27, -14, 37, -24, 22, -13, 39, -12, 41, -11, 43, -10, 45, -9, 47,
    -8, 49, -7, 51, -6, 53, -5, 55, -4, 57,
};

const unsigned axis1_wave27_shadow_unsigned_offsets[42] = {
    27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u, 67u, 71u, 75u, 79u, 83u, 87u,
    91u, 95u, 99u, 103u, 107u, 111u, 115u, 119u, 123u, 127u, 131u, 135u, 139u, 143u, 147u, 151u,
    155u, 159u, 163u, 167u, 171u, 175u, 179u, 183u, 187u, 191u,
};

const unsigned* axis1_wave27_shadow_windows[42] = {
    axis1_wave27_shadow_weights + 0, axis1_wave27_shadow_weights + 2,
    axis1_wave27_shadow_weights + 4, axis1_wave27_shadow_weights + 6,
    axis1_wave27_shadow_weights + 8, axis1_wave27_shadow_weights + 10,
    axis1_wave27_shadow_weights + 12, axis1_wave27_shadow_weights + 14,
    axis1_wave27_shadow_weights + 16, axis1_wave27_shadow_weights + 18,
    axis1_wave27_shadow_weights + 20, axis1_wave27_shadow_weights + 22,
    axis1_wave27_shadow_weights + 24, axis1_wave27_shadow_weights + 26,
    axis1_wave27_shadow_weights + 28, axis1_wave27_shadow_weights + 30,
    axis1_wave27_shadow_weights + 32, axis1_wave27_shadow_weights + 34,
    axis1_wave27_shadow_weights + 1, axis1_wave27_shadow_weights + 3,
    axis1_wave27_shadow_weights + 5, axis1_wave27_shadow_weights + 7,
    axis1_wave27_shadow_weights + 9, axis1_wave27_shadow_weights + 11,
    axis1_wave27_shadow_weights + 13, axis1_wave27_shadow_weights + 15,
    axis1_wave27_shadow_weights + 17, axis1_wave27_shadow_weights + 19,
    axis1_wave27_shadow_weights + 21, axis1_wave27_shadow_weights + 23,
    axis1_wave27_shadow_weights + 25, axis1_wave27_shadow_weights + 27,
    axis1_wave27_shadow_weights + 29, axis1_wave27_shadow_weights + 31,
    axis1_wave27_shadow_weights + 33, axis1_wave27_shadow_weights + 35,
    axis1_wave27_shadow_weights + 37, axis1_wave27_shadow_weights + 39,
    axis1_wave27_shadow_weights + 41, axis1_wave27_shadow_weights + 43,
    axis1_wave27_shadow_weights + 45, axis1_wave27_shadow_weights + 47,
};

const unsigned** axis1_wave27_shadow_routes[16] = {
    axis1_wave27_shadow_windows + 0, axis1_wave27_shadow_windows + 2,
    axis1_wave27_shadow_windows + 4, axis1_wave27_shadow_windows + 6,
    axis1_wave27_shadow_windows + 8, axis1_wave27_shadow_windows + 10,
    axis1_wave27_shadow_windows + 12, axis1_wave27_shadow_windows + 14,
    axis1_wave27_shadow_windows + 16, axis1_wave27_shadow_windows + 18,
    axis1_wave27_shadow_windows + 20, axis1_wave27_shadow_windows + 22,
    axis1_wave27_shadow_windows + 24, axis1_wave27_shadow_windows + 26,
    axis1_wave27_shadow_windows + 28, axis1_wave27_shadow_windows + 30,
};

const unsigned*** axis1_wave27_shadow_plans[4] = {
    axis1_wave27_shadow_routes + 0,
    axis1_wave27_shadow_routes + 4,
    axis1_wave27_shadow_routes + 8,
    axis1_wave27_shadow_routes + 12,
};
