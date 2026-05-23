const unsigned axis1_wave21_collapse_weights[60] = {
    18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u,
    66u, 70u, 74u, 78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u, 110u,
    114u, 118u, 122u, 126u, 130u, 134u, 138u, 142u, 146u, 150u, 154u, 158u,
    162u, 166u, 170u, 174u, 178u, 182u, 186u, 190u, 194u, 198u, 202u, 206u,
    210u, 214u, 218u, 222u, 226u, 230u, 234u, 238u, 242u, 246u, 250u, 254u,
};

const int axis1_wave21_collapse_signed_offsets[30] = {
    -17, 10, -14, 16, -11, 19, -15, 12, -9, 21, -16, 13, -12, 23, -10,
    25, -13, 20, -8, 26, -18, 15, -7, 27, -6, 29, -5, 30, -4, 31,
};

const unsigned axis1_wave21_collapse_unsigned_offsets[30] = {
    15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u, 67u, 71u,
    75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u, 107u, 111u, 115u, 119u, 123u, 127u, 131u,
};

const unsigned* axis1_wave21_collapse_windows[30] = {
    axis1_wave21_collapse_weights + 0, axis1_wave21_collapse_weights + 2,
    axis1_wave21_collapse_weights + 4, axis1_wave21_collapse_weights + 6,
    axis1_wave21_collapse_weights + 8, axis1_wave21_collapse_weights + 10,
    axis1_wave21_collapse_weights + 12, axis1_wave21_collapse_weights + 14,
    axis1_wave21_collapse_weights + 16, axis1_wave21_collapse_weights + 18,
    axis1_wave21_collapse_weights + 20, axis1_wave21_collapse_weights + 22,
    axis1_wave21_collapse_weights + 24, axis1_wave21_collapse_weights + 26,
    axis1_wave21_collapse_weights + 28, axis1_wave21_collapse_weights + 30,
    axis1_wave21_collapse_weights + 32, axis1_wave21_collapse_weights + 34,
    axis1_wave21_collapse_weights + 36, axis1_wave21_collapse_weights + 38,
    axis1_wave21_collapse_weights + 40, axis1_wave21_collapse_weights + 42,
    axis1_wave21_collapse_weights + 44, axis1_wave21_collapse_weights + 46,
    axis1_wave21_collapse_weights + 48, axis1_wave21_collapse_weights + 50,
    axis1_wave21_collapse_weights + 52, axis1_wave21_collapse_weights + 54,
    axis1_wave21_collapse_weights + 56, axis1_wave21_collapse_weights + 58,
};

const unsigned** axis1_wave21_collapse_routes[15] = {
    axis1_wave21_collapse_windows + 0, axis1_wave21_collapse_windows + 2,
    axis1_wave21_collapse_windows + 4, axis1_wave21_collapse_windows + 6,
    axis1_wave21_collapse_windows + 8, axis1_wave21_collapse_windows + 10,
    axis1_wave21_collapse_windows + 12, axis1_wave21_collapse_windows + 1,
    axis1_wave21_collapse_windows + 3, axis1_wave21_collapse_windows + 5,
    axis1_wave21_collapse_windows + 7, axis1_wave21_collapse_windows + 9,
    axis1_wave21_collapse_windows + 11, axis1_wave21_collapse_windows + 13,
    axis1_wave21_collapse_windows + 15,
};

const unsigned*** axis1_wave21_collapse_plans[4] = {
    axis1_wave21_collapse_routes + 0,
    axis1_wave21_collapse_routes + 4,
    axis1_wave21_collapse_routes + 8,
    axis1_wave21_collapse_routes + 11,
};
