const unsigned axis1_wave19_window_weights[54] = {
    15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u,
    63u, 67u, 71u, 75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u, 107u,
    111u, 115u, 119u, 123u, 127u, 131u, 135u, 139u, 143u, 147u, 151u, 155u,
    159u, 163u, 167u, 171u, 175u, 179u, 183u, 187u, 191u, 195u, 199u, 203u,
    207u, 211u, 215u, 219u, 223u, 227u,
};

const unsigned axis1_wave19_window_lane_masks[13] = {
    3u, 6u, 9u, 5u, 10u, 12u, 7u, 11u, 13u, 14u, 8u, 15u, 17u,
};

const int axis1_wave19_window_signed_offsets[24] = {
    -14, 9, -11, 15, -8, 17, -12, 10, -6, 19, -13, 11,
    -9, 21, -7, 23, -10, 18, -5, 24, -15, 13, -4, 25,
};

const unsigned axis1_wave19_window_unsigned_offsets[24] = {
    12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u,
    60u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u,
};

const unsigned* axis1_wave19_window_windows[27] = {
    axis1_wave19_window_weights + 0, axis1_wave19_window_weights + 2,
    axis1_wave19_window_weights + 4, axis1_wave19_window_weights + 6,
    axis1_wave19_window_weights + 8, axis1_wave19_window_weights + 10,
    axis1_wave19_window_weights + 12, axis1_wave19_window_weights + 14,
    axis1_wave19_window_weights + 16, axis1_wave19_window_weights + 18,
    axis1_wave19_window_weights + 20, axis1_wave19_window_weights + 22,
    axis1_wave19_window_weights + 24, axis1_wave19_window_weights + 26,
    axis1_wave19_window_weights + 28, axis1_wave19_window_weights + 30,
    axis1_wave19_window_weights + 32, axis1_wave19_window_weights + 34,
    axis1_wave19_window_weights + 36, axis1_wave19_window_weights + 38,
    axis1_wave19_window_weights + 40, axis1_wave19_window_weights + 42,
    axis1_wave19_window_weights + 44, axis1_wave19_window_weights + 46,
    axis1_wave19_window_weights + 48, axis1_wave19_window_weights + 50,
    axis1_wave19_window_weights + 52,
};

const unsigned** axis1_wave19_window_routes[13] = {
    axis1_wave19_window_windows + 0, axis1_wave19_window_windows + 2,
    axis1_wave19_window_windows + 4, axis1_wave19_window_windows + 6,
    axis1_wave19_window_windows + 8, axis1_wave19_window_windows + 10,
    axis1_wave19_window_windows + 12, axis1_wave19_window_windows + 1,
    axis1_wave19_window_windows + 3, axis1_wave19_window_windows + 5,
    axis1_wave19_window_windows + 7, axis1_wave19_window_windows + 9,
    axis1_wave19_window_windows + 11,
};

const unsigned*** axis1_wave19_window_plans[3] = {
    axis1_wave19_window_routes + 0,
    axis1_wave19_window_routes + 4,
    axis1_wave19_window_routes + 8,
};
