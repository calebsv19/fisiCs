const unsigned axis1_wave21_braid_weights[62] = {
    19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u,
    67u, 71u, 75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u, 107u, 111u,
    115u, 119u, 123u, 127u, 131u, 135u, 139u, 143u, 147u, 151u, 155u, 159u,
    163u, 167u, 171u, 175u, 179u, 183u, 187u, 191u, 195u, 199u, 203u, 207u,
    211u, 215u, 219u, 223u, 227u, 231u, 235u, 239u, 243u, 247u, 251u, 255u,
    259u, 263u,
};

const unsigned axis1_wave21_braid_lane_masks[15] = {
    3u, 6u, 9u, 5u, 10u, 12u, 7u, 11u, 13u, 14u, 8u, 15u, 17u, 19u, 21u,
};

const int axis1_wave21_braid_signed_offsets[28] = {
    -16, 11, -13, 17, -10, 19, -14, 12, -8, 21, -15, 13, -11, 23,
    -9, 25, -12, 20, -7, 26, -17, 15, -6, 27, -5, 29, -4, 31,
};

const unsigned axis1_wave21_braid_unsigned_offsets[28] = {
    16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u, 60u, 64u, 68u,
    72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u, 108u, 112u, 116u, 120u, 124u,
};

const unsigned* axis1_wave21_braid_windows[31] = {
    axis1_wave21_braid_weights + 0, axis1_wave21_braid_weights + 2,
    axis1_wave21_braid_weights + 4, axis1_wave21_braid_weights + 6,
    axis1_wave21_braid_weights + 8, axis1_wave21_braid_weights + 10,
    axis1_wave21_braid_weights + 12, axis1_wave21_braid_weights + 14,
    axis1_wave21_braid_weights + 16, axis1_wave21_braid_weights + 18,
    axis1_wave21_braid_weights + 20, axis1_wave21_braid_weights + 22,
    axis1_wave21_braid_weights + 24, axis1_wave21_braid_weights + 26,
    axis1_wave21_braid_weights + 28, axis1_wave21_braid_weights + 30,
    axis1_wave21_braid_weights + 32, axis1_wave21_braid_weights + 34,
    axis1_wave21_braid_weights + 36, axis1_wave21_braid_weights + 38,
    axis1_wave21_braid_weights + 40, axis1_wave21_braid_weights + 42,
    axis1_wave21_braid_weights + 44, axis1_wave21_braid_weights + 46,
    axis1_wave21_braid_weights + 48, axis1_wave21_braid_weights + 50,
    axis1_wave21_braid_weights + 52, axis1_wave21_braid_weights + 54,
    axis1_wave21_braid_weights + 56, axis1_wave21_braid_weights + 58,
    axis1_wave21_braid_weights + 60,
};

const unsigned** axis1_wave21_braid_routes[15] = {
    axis1_wave21_braid_windows + 0, axis1_wave21_braid_windows + 2,
    axis1_wave21_braid_windows + 4, axis1_wave21_braid_windows + 6,
    axis1_wave21_braid_windows + 8, axis1_wave21_braid_windows + 10,
    axis1_wave21_braid_windows + 12, axis1_wave21_braid_windows + 1,
    axis1_wave21_braid_windows + 3, axis1_wave21_braid_windows + 5,
    axis1_wave21_braid_windows + 7, axis1_wave21_braid_windows + 9,
    axis1_wave21_braid_windows + 11, axis1_wave21_braid_windows + 13,
    axis1_wave21_braid_windows + 15,
};

const unsigned*** axis1_wave21_braid_plans[3] = {
    axis1_wave21_braid_routes + 0,
    axis1_wave21_braid_routes + 5,
    axis1_wave21_braid_routes + 10,
};
