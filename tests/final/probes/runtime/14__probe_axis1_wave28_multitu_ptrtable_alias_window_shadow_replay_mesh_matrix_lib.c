const unsigned axis1_wave28_shadow_replay_weights[90] = {
    38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u, 78u, 82u,
    86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u, 122u, 126u, 130u,
    134u, 138u, 142u, 146u, 150u, 154u, 158u, 162u, 166u, 170u, 174u, 178u,
    182u, 186u, 190u, 194u, 198u, 202u, 206u, 210u, 214u, 218u, 222u, 226u,
    230u, 234u, 238u, 242u, 246u, 250u, 254u, 258u, 262u, 266u, 270u, 274u,
    278u, 282u, 286u, 290u, 294u, 298u, 302u, 306u, 310u, 314u, 318u, 322u,
    326u, 330u, 334u, 338u, 342u, 346u, 350u, 354u, 358u, 362u, 366u, 370u,
    374u, 378u, 382u, 386u, 390u, 394u
};

const unsigned axis1_wave28_shadow_replay_lane_masks[24] = {
    10u, 14u, 11u, 15u, 12u, 16u, 13u, 17u, 18u, 19u, 20u, 21u,
    22u, 24u, 26u, 28u, 30u, 32u, 34u, 36u, 38u, 40u, 42u, 44u
};

const int axis1_wave28_shadow_replay_signed_offsets[42] = {
    -23, 18, -20, 25, -17, 28, -22, 19, -15, 31, -21, 20, -18, 33, -16, 35,
    -19, 27, -14, 37, -24, 22, -13, 39, -12, 41, -11, 43, -10, 45, -9, 47,
    -8, 49, -7, 51, -6, 53, -5, 55, -4, 57
};

const unsigned axis1_wave28_shadow_replay_unsigned_offsets[42] = {
    30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u,
    78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u, 122u,
    126u, 130u, 134u, 138u, 142u, 146u, 150u, 154u, 158u, 162u, 166u, 170u,
    174u, 178u, 182u, 186u, 190u, 194u
};

const unsigned* axis1_wave28_shadow_replay_windows[45] = {
    axis1_wave28_shadow_replay_weights + 0, axis1_wave28_shadow_replay_weights + 2,
    axis1_wave28_shadow_replay_weights + 4, axis1_wave28_shadow_replay_weights + 6,
    axis1_wave28_shadow_replay_weights + 8, axis1_wave28_shadow_replay_weights + 10,
    axis1_wave28_shadow_replay_weights + 12, axis1_wave28_shadow_replay_weights + 14,
    axis1_wave28_shadow_replay_weights + 16, axis1_wave28_shadow_replay_weights + 18,
    axis1_wave28_shadow_replay_weights + 20, axis1_wave28_shadow_replay_weights + 22,
    axis1_wave28_shadow_replay_weights + 24, axis1_wave28_shadow_replay_weights + 26,
    axis1_wave28_shadow_replay_weights + 28, axis1_wave28_shadow_replay_weights + 30,
    axis1_wave28_shadow_replay_weights + 32, axis1_wave28_shadow_replay_weights + 34,
    axis1_wave28_shadow_replay_weights + 1, axis1_wave28_shadow_replay_weights + 3,
    axis1_wave28_shadow_replay_weights + 5, axis1_wave28_shadow_replay_weights + 7,
    axis1_wave28_shadow_replay_weights + 9, axis1_wave28_shadow_replay_weights + 11,
    axis1_wave28_shadow_replay_weights + 13, axis1_wave28_shadow_replay_weights + 15,
    axis1_wave28_shadow_replay_weights + 17, axis1_wave28_shadow_replay_weights + 19,
    axis1_wave28_shadow_replay_weights + 21, axis1_wave28_shadow_replay_weights + 23,
    axis1_wave28_shadow_replay_weights + 25, axis1_wave28_shadow_replay_weights + 27,
    axis1_wave28_shadow_replay_weights + 29, axis1_wave28_shadow_replay_weights + 31,
    axis1_wave28_shadow_replay_weights + 33, axis1_wave28_shadow_replay_weights + 35,
    axis1_wave28_shadow_replay_weights + 37, axis1_wave28_shadow_replay_weights + 39,
    axis1_wave28_shadow_replay_weights + 41, axis1_wave28_shadow_replay_weights + 43,
    axis1_wave28_shadow_replay_weights + 45, axis1_wave28_shadow_replay_weights + 47,
    axis1_wave28_shadow_replay_weights + 49, axis1_wave28_shadow_replay_weights + 51,
    axis1_wave28_shadow_replay_weights + 53
};

const unsigned** axis1_wave28_shadow_replay_routes[16] = {
    axis1_wave28_shadow_replay_windows + 0, axis1_wave28_shadow_replay_windows + 2,
    axis1_wave28_shadow_replay_windows + 4, axis1_wave28_shadow_replay_windows + 6,
    axis1_wave28_shadow_replay_windows + 8, axis1_wave28_shadow_replay_windows + 10,
    axis1_wave28_shadow_replay_windows + 12, axis1_wave28_shadow_replay_windows + 14,
    axis1_wave28_shadow_replay_windows + 16, axis1_wave28_shadow_replay_windows + 18,
    axis1_wave28_shadow_replay_windows + 20, axis1_wave28_shadow_replay_windows + 22,
    axis1_wave28_shadow_replay_windows + 24, axis1_wave28_shadow_replay_windows + 26,
    axis1_wave28_shadow_replay_windows + 28, axis1_wave28_shadow_replay_windows + 30
};

const unsigned*** axis1_wave28_shadow_replay_plans[3] = {
    axis1_wave28_shadow_replay_routes + 0,
    axis1_wave28_shadow_replay_routes + 5,
    axis1_wave28_shadow_replay_routes + 10
};
