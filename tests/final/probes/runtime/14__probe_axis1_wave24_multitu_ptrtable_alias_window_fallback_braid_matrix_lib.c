const unsigned axis1_wave24_braid_weights[74] = {
    27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u, 67u, 71u,
    75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u, 107u, 111u, 115u, 119u,
    123u, 127u, 131u, 135u, 139u, 143u, 147u, 151u, 155u, 159u, 163u, 167u,
    171u, 175u, 179u, 183u, 187u, 191u, 195u, 199u, 203u, 207u, 211u, 215u,
    219u, 223u, 227u, 231u, 235u, 239u, 243u, 247u, 251u, 255u, 259u, 263u,
    267u, 271u, 275u, 279u, 283u, 287u, 291u, 295u, 299u, 303u, 307u, 311u,
    315u, 319u,
};

const unsigned axis1_wave24_braid_lane_masks[20] = {
    6u, 10u, 7u, 11u, 8u, 12u, 9u, 13u, 14u, 15u, 16u, 17u, 18u, 20u, 22u, 24u, 26u, 28u, 30u, 32u,
};

const int axis1_wave24_braid_signed_offsets[34] = {
    -19, 14, -16, 21, -13, 24, -18, 15, -11, 27, -17, 16, -14, 29, -12, 31,
    -15, 23, -10, 33, -20, 18, -9, 35, -8, 37, -7, 39, -6, 41, -5, 43, -4, 45,
};

const unsigned axis1_wave24_braid_unsigned_offsets[34] = {
    22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u, 78u, 82u,
    86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u, 122u, 126u, 130u, 134u, 138u, 142u, 146u,
    150u, 154u,
};

const unsigned* axis1_wave24_braid_windows[37] = {
    axis1_wave24_braid_weights + 0, axis1_wave24_braid_weights + 2,
    axis1_wave24_braid_weights + 4, axis1_wave24_braid_weights + 6,
    axis1_wave24_braid_weights + 8, axis1_wave24_braid_weights + 10,
    axis1_wave24_braid_weights + 12, axis1_wave24_braid_weights + 14,
    axis1_wave24_braid_weights + 16, axis1_wave24_braid_weights + 18,
    axis1_wave24_braid_weights + 20, axis1_wave24_braid_weights + 22,
    axis1_wave24_braid_weights + 24, axis1_wave24_braid_weights + 26,
    axis1_wave24_braid_weights + 28, axis1_wave24_braid_weights + 30,
    axis1_wave24_braid_weights + 32, axis1_wave24_braid_weights + 34,
    axis1_wave24_braid_weights + 1, axis1_wave24_braid_weights + 3,
    axis1_wave24_braid_weights + 5, axis1_wave24_braid_weights + 7,
    axis1_wave24_braid_weights + 9, axis1_wave24_braid_weights + 11,
    axis1_wave24_braid_weights + 13, axis1_wave24_braid_weights + 15,
    axis1_wave24_braid_weights + 17, axis1_wave24_braid_weights + 19,
    axis1_wave24_braid_weights + 21, axis1_wave24_braid_weights + 23,
    axis1_wave24_braid_weights + 25, axis1_wave24_braid_weights + 27,
    axis1_wave24_braid_weights + 29, axis1_wave24_braid_weights + 31,
    axis1_wave24_braid_weights + 33, axis1_wave24_braid_weights + 35,
    axis1_wave24_braid_weights + 37,
};

const unsigned** axis1_wave24_braid_routes[16] = {
    axis1_wave24_braid_windows + 0, axis1_wave24_braid_windows + 2,
    axis1_wave24_braid_windows + 4, axis1_wave24_braid_windows + 6,
    axis1_wave24_braid_windows + 8, axis1_wave24_braid_windows + 10,
    axis1_wave24_braid_windows + 12, axis1_wave24_braid_windows + 14,
    axis1_wave24_braid_windows + 16, axis1_wave24_braid_windows + 18,
    axis1_wave24_braid_windows + 20, axis1_wave24_braid_windows + 22,
    axis1_wave24_braid_windows + 24, axis1_wave24_braid_windows + 26,
    axis1_wave24_braid_windows + 28, axis1_wave24_braid_windows + 30,
};

const unsigned*** axis1_wave24_braid_plans[3] = {
    axis1_wave24_braid_routes + 0,
    axis1_wave24_braid_routes + 5,
    axis1_wave24_braid_routes + 10,
};
