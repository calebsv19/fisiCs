const unsigned axis1_wave22_braid_weights[66] = {
    21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u,
    69u, 73u, 77u, 81u, 85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u,
    117u, 121u, 125u, 129u, 133u, 137u, 141u, 145u, 149u, 153u, 157u, 161u,
    165u, 169u, 173u, 177u, 181u, 185u, 189u, 193u, 197u, 201u, 205u, 209u,
    213u, 217u, 221u, 225u, 229u, 233u, 237u, 241u, 245u, 249u, 253u, 257u,
    261u, 265u, 269u, 273u, 277u, 281u,
};

const unsigned axis1_wave22_braid_lane_masks[16] = {
    3u, 6u, 9u, 5u, 10u, 12u, 7u, 11u, 13u, 14u, 8u, 15u, 17u, 19u, 21u, 23u,
};

const int axis1_wave22_braid_signed_offsets[30] = {
    -17, 12, -14, 18, -11, 20, -15, 13, -9, 22, -16, 14, -12, 24, -10,
    26, -13, 21, -8, 27, -18, 16, -7, 28, -6, 30, -5, 32, -4, 33,
};

const unsigned axis1_wave22_braid_unsigned_offsets[30] = {
    18u, 22u, 26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u,
    78u, 82u, 86u, 90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u, 122u, 126u, 130u, 134u,
};

const unsigned* axis1_wave22_braid_windows[33] = {
    axis1_wave22_braid_weights + 0, axis1_wave22_braid_weights + 2,
    axis1_wave22_braid_weights + 4, axis1_wave22_braid_weights + 6,
    axis1_wave22_braid_weights + 8, axis1_wave22_braid_weights + 10,
    axis1_wave22_braid_weights + 12, axis1_wave22_braid_weights + 14,
    axis1_wave22_braid_weights + 16, axis1_wave22_braid_weights + 18,
    axis1_wave22_braid_weights + 20, axis1_wave22_braid_weights + 22,
    axis1_wave22_braid_weights + 24, axis1_wave22_braid_weights + 26,
    axis1_wave22_braid_weights + 28, axis1_wave22_braid_weights + 30,
    axis1_wave22_braid_weights + 32, axis1_wave22_braid_weights + 34,
    axis1_wave22_braid_weights + 36, axis1_wave22_braid_weights + 38,
    axis1_wave22_braid_weights + 40, axis1_wave22_braid_weights + 42,
    axis1_wave22_braid_weights + 44, axis1_wave22_braid_weights + 46,
    axis1_wave22_braid_weights + 48, axis1_wave22_braid_weights + 50,
    axis1_wave22_braid_weights + 52, axis1_wave22_braid_weights + 54,
    axis1_wave22_braid_weights + 56, axis1_wave22_braid_weights + 58,
    axis1_wave22_braid_weights + 60, axis1_wave22_braid_weights + 62,
    axis1_wave22_braid_weights + 64,
};

const unsigned** axis1_wave22_braid_routes[16] = {
    axis1_wave22_braid_windows + 0, axis1_wave22_braid_windows + 2,
    axis1_wave22_braid_windows + 4, axis1_wave22_braid_windows + 6,
    axis1_wave22_braid_windows + 8, axis1_wave22_braid_windows + 10,
    axis1_wave22_braid_windows + 12, axis1_wave22_braid_windows + 14,
    axis1_wave22_braid_windows + 1, axis1_wave22_braid_windows + 3,
    axis1_wave22_braid_windows + 5, axis1_wave22_braid_windows + 7,
    axis1_wave22_braid_windows + 9, axis1_wave22_braid_windows + 11,
    axis1_wave22_braid_windows + 13, axis1_wave22_braid_windows + 15,
};

const unsigned*** axis1_wave22_braid_plans[3] = {
    axis1_wave22_braid_routes + 0,
    axis1_wave22_braid_routes + 6,
    axis1_wave22_braid_routes + 11,
};
