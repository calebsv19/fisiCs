const unsigned axis1_wave25_braid_weights[78] = {
    30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u,
    78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u, 122u,
    126u, 130u, 134u, 138u, 142u, 146u, 150u, 154u, 158u, 162u, 166u, 170u,
    174u, 178u, 182u, 186u, 190u, 194u, 198u, 202u, 206u, 210u, 214u, 218u,
    222u, 226u, 230u, 234u, 238u, 242u, 246u, 250u, 254u, 258u, 262u, 266u,
    270u, 274u, 278u, 282u, 286u, 290u, 294u, 298u, 302u, 306u, 310u, 314u,
    318u, 322u, 326u, 330u, 334u, 338u,
};

const unsigned axis1_wave25_braid_lane_masks[20] = {
    7u, 11u, 8u, 12u, 9u, 13u, 10u, 14u, 15u, 16u, 17u, 18u, 19u, 21u, 23u, 25u, 27u, 29u, 31u, 33u,
};

const int axis1_wave25_braid_signed_offsets[36] = {
    -20, 15, -17, 22, -14, 25, -19, 16, -12, 28, -18, 17, -15, 30, -13, 32,
    -16, 24, -11, 34, -21, 19, -10, 36, -9, 38, -8, 40, -7, 42, -6, 44, -5, 46, -4, 48,
};

const unsigned axis1_wave25_braid_unsigned_offsets[36] = {
    24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u, 60u, 64u, 68u, 72u, 76u, 80u, 84u,
    88u, 92u, 96u, 100u, 104u, 108u, 112u, 116u, 120u, 124u, 128u, 132u, 136u, 140u, 144u, 148u,
    152u, 156u, 160u, 164u,
};

const unsigned* axis1_wave25_braid_windows[39] = {
    axis1_wave25_braid_weights + 0, axis1_wave25_braid_weights + 2,
    axis1_wave25_braid_weights + 4, axis1_wave25_braid_weights + 6,
    axis1_wave25_braid_weights + 8, axis1_wave25_braid_weights + 10,
    axis1_wave25_braid_weights + 12, axis1_wave25_braid_weights + 14,
    axis1_wave25_braid_weights + 16, axis1_wave25_braid_weights + 18,
    axis1_wave25_braid_weights + 20, axis1_wave25_braid_weights + 22,
    axis1_wave25_braid_weights + 24, axis1_wave25_braid_weights + 26,
    axis1_wave25_braid_weights + 28, axis1_wave25_braid_weights + 30,
    axis1_wave25_braid_weights + 32, axis1_wave25_braid_weights + 34,
    axis1_wave25_braid_weights + 1, axis1_wave25_braid_weights + 3,
    axis1_wave25_braid_weights + 5, axis1_wave25_braid_weights + 7,
    axis1_wave25_braid_weights + 9, axis1_wave25_braid_weights + 11,
    axis1_wave25_braid_weights + 13, axis1_wave25_braid_weights + 15,
    axis1_wave25_braid_weights + 17, axis1_wave25_braid_weights + 19,
    axis1_wave25_braid_weights + 21, axis1_wave25_braid_weights + 23,
    axis1_wave25_braid_weights + 25, axis1_wave25_braid_weights + 27,
    axis1_wave25_braid_weights + 29, axis1_wave25_braid_weights + 31,
    axis1_wave25_braid_weights + 33, axis1_wave25_braid_weights + 35,
    axis1_wave25_braid_weights + 37, axis1_wave25_braid_weights + 39,
};

const unsigned** axis1_wave25_braid_routes[16] = {
    axis1_wave25_braid_windows + 0, axis1_wave25_braid_windows + 2,
    axis1_wave25_braid_windows + 4, axis1_wave25_braid_windows + 6,
    axis1_wave25_braid_windows + 8, axis1_wave25_braid_windows + 10,
    axis1_wave25_braid_windows + 12, axis1_wave25_braid_windows + 14,
    axis1_wave25_braid_windows + 16, axis1_wave25_braid_windows + 18,
    axis1_wave25_braid_windows + 20, axis1_wave25_braid_windows + 22,
    axis1_wave25_braid_windows + 24, axis1_wave25_braid_windows + 26,
    axis1_wave25_braid_windows + 28, axis1_wave25_braid_windows + 30,
};

const unsigned*** axis1_wave25_braid_plans[3] = {
    axis1_wave25_braid_routes + 0,
    axis1_wave25_braid_routes + 5,
    axis1_wave25_braid_routes + 10,
};
