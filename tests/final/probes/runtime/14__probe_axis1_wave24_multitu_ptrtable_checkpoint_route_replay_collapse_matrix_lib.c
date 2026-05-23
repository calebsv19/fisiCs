const unsigned axis1_wave24_replay_weights[72] = {
    26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u,
    74u, 78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u,
    122u, 126u, 130u, 134u, 138u, 142u, 146u, 150u, 154u, 158u, 162u, 166u,
    170u, 174u, 178u, 182u, 186u, 190u, 194u, 198u, 202u, 206u, 210u, 214u,
    218u, 222u, 226u, 230u, 234u, 238u, 242u, 246u, 250u, 254u, 258u, 262u,
    266u, 270u, 274u, 278u, 282u, 286u, 290u, 294u, 298u, 302u, 306u, 310u,
};

const int axis1_wave24_replay_signed_offsets[36] = {
    -20, 15, -17, 22, -14, 25, -19, 16, -12, 28, -18, 17, -15, 30, -13, 32,
    -16, 24, -11, 34, -21, 19, -10, 36, -9, 38, -8, 40, -7, 42, -6, 44, -5, 46, -4, 48,
};

const unsigned axis1_wave24_replay_unsigned_offsets[36] = {
    21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u,
    85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u, 125u, 129u, 133u, 137u, 141u, 145u,
    149u, 153u, 157u, 161u,
};

const unsigned* axis1_wave24_replay_windows[36] = {
    axis1_wave24_replay_weights + 0, axis1_wave24_replay_weights + 2,
    axis1_wave24_replay_weights + 4, axis1_wave24_replay_weights + 6,
    axis1_wave24_replay_weights + 8, axis1_wave24_replay_weights + 10,
    axis1_wave24_replay_weights + 12, axis1_wave24_replay_weights + 14,
    axis1_wave24_replay_weights + 16, axis1_wave24_replay_weights + 18,
    axis1_wave24_replay_weights + 20, axis1_wave24_replay_weights + 22,
    axis1_wave24_replay_weights + 24, axis1_wave24_replay_weights + 26,
    axis1_wave24_replay_weights + 28, axis1_wave24_replay_weights + 30,
    axis1_wave24_replay_weights + 32, axis1_wave24_replay_weights + 34,
    axis1_wave24_replay_weights + 1, axis1_wave24_replay_weights + 3,
    axis1_wave24_replay_weights + 5, axis1_wave24_replay_weights + 7,
    axis1_wave24_replay_weights + 9, axis1_wave24_replay_weights + 11,
    axis1_wave24_replay_weights + 13, axis1_wave24_replay_weights + 15,
    axis1_wave24_replay_weights + 17, axis1_wave24_replay_weights + 19,
    axis1_wave24_replay_weights + 21, axis1_wave24_replay_weights + 23,
    axis1_wave24_replay_weights + 25, axis1_wave24_replay_weights + 27,
    axis1_wave24_replay_weights + 29, axis1_wave24_replay_weights + 31,
    axis1_wave24_replay_weights + 33, axis1_wave24_replay_weights + 35,
};

const unsigned** axis1_wave24_replay_routes[16] = {
    axis1_wave24_replay_windows + 0, axis1_wave24_replay_windows + 2,
    axis1_wave24_replay_windows + 4, axis1_wave24_replay_windows + 6,
    axis1_wave24_replay_windows + 8, axis1_wave24_replay_windows + 10,
    axis1_wave24_replay_windows + 12, axis1_wave24_replay_windows + 14,
    axis1_wave24_replay_windows + 16, axis1_wave24_replay_windows + 18,
    axis1_wave24_replay_windows + 20, axis1_wave24_replay_windows + 22,
    axis1_wave24_replay_windows + 24, axis1_wave24_replay_windows + 26,
    axis1_wave24_replay_windows + 28, axis1_wave24_replay_windows + 30,
};

const unsigned*** axis1_wave24_replay_plans[4] = {
    axis1_wave24_replay_routes + 0,
    axis1_wave24_replay_routes + 4,
    axis1_wave24_replay_routes + 8,
    axis1_wave24_replay_routes + 12,
};
