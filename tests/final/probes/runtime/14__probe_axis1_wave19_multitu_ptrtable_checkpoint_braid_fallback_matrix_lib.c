const unsigned axis1_wave19_braid_weights[52] = {
    14u, 18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u,
    62u, 66u, 70u, 74u, 78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u,
    110u, 114u, 118u, 122u, 126u, 130u, 134u, 138u, 142u, 146u, 150u, 154u,
    158u, 162u, 166u, 170u, 174u, 178u, 182u, 186u, 190u, 194u, 198u, 202u,
    206u, 210u, 214u, 218u,
};

const int axis1_wave19_braid_signed_offsets[26] = {
    -15, 8, -12, 14, -9, 17, -13, 10, -7, 19, -14, 11, -10,
    21, -8, 23, -11, 18, -6, 24, -16, 13, -5, 25, -4, 27,
};

const unsigned axis1_wave19_braid_unsigned_offsets[26] = {
    11u, 15u, 19u, 23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u,
    63u, 67u, 71u, 75u, 79u, 83u, 87u, 91u, 95u, 99u, 103u, 107u, 111u,
};

const unsigned* axis1_wave19_braid_windows[26] = {
    axis1_wave19_braid_weights + 0, axis1_wave19_braid_weights + 2,
    axis1_wave19_braid_weights + 4, axis1_wave19_braid_weights + 6,
    axis1_wave19_braid_weights + 8, axis1_wave19_braid_weights + 10,
    axis1_wave19_braid_weights + 12, axis1_wave19_braid_weights + 14,
    axis1_wave19_braid_weights + 16, axis1_wave19_braid_weights + 18,
    axis1_wave19_braid_weights + 20, axis1_wave19_braid_weights + 22,
    axis1_wave19_braid_weights + 24, axis1_wave19_braid_weights + 26,
    axis1_wave19_braid_weights + 28, axis1_wave19_braid_weights + 30,
    axis1_wave19_braid_weights + 32, axis1_wave19_braid_weights + 34,
    axis1_wave19_braid_weights + 36, axis1_wave19_braid_weights + 38,
    axis1_wave19_braid_weights + 40, axis1_wave19_braid_weights + 42,
    axis1_wave19_braid_weights + 44, axis1_wave19_braid_weights + 46,
    axis1_wave19_braid_weights + 48, axis1_wave19_braid_weights + 50,
};

const unsigned** axis1_wave19_braid_routes[13] = {
    axis1_wave19_braid_windows + 0, axis1_wave19_braid_windows + 2,
    axis1_wave19_braid_windows + 4, axis1_wave19_braid_windows + 6,
    axis1_wave19_braid_windows + 8, axis1_wave19_braid_windows + 10,
    axis1_wave19_braid_windows + 12, axis1_wave19_braid_windows + 1,
    axis1_wave19_braid_windows + 3, axis1_wave19_braid_windows + 5,
    axis1_wave19_braid_windows + 7, axis1_wave19_braid_windows + 9,
    axis1_wave19_braid_windows + 11,
};

const unsigned*** axis1_wave19_braid_plans[4] = {
    axis1_wave19_braid_routes + 0,
    axis1_wave19_braid_routes + 3,
    axis1_wave19_braid_routes + 6,
    axis1_wave19_braid_routes + 9,
};
