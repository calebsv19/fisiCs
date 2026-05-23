const unsigned axis1_wave26_mesh_weights[82] = {
    33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u,
    81u, 85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u, 125u,
    129u, 133u, 137u, 141u, 145u, 149u, 153u, 157u, 161u, 165u, 169u, 173u,
    177u, 181u, 185u, 189u, 193u, 197u, 201u, 205u, 209u, 213u, 217u, 221u,
    225u, 229u, 233u, 237u, 241u, 245u, 249u, 253u, 257u, 261u, 265u, 269u,
    273u, 277u, 281u, 285u, 289u, 293u, 297u, 301u, 305u, 309u, 313u, 317u,
    321u, 325u, 329u, 333u, 337u, 341u, 345u, 349u, 353u, 357u,
};

const unsigned axis1_wave26_mesh_lane_masks[22] = {
    8u, 12u, 9u, 13u, 10u, 14u, 11u, 15u, 16u, 17u, 18u, 19u, 20u, 22u, 24u, 26u, 28u, 30u, 32u, 34u, 36u, 38u,
};

const int axis1_wave26_mesh_signed_offsets[38] = {
    -21, 16, -18, 23, -15, 26, -20, 17, -13, 29, -19, 18, -16, 31, -14, 33,
    -17, 25, -12, 35, -22, 20, -11, 37, -10, 39, -9, 41, -8, 43, -7, 45, -6, 47, -5, 49, -4, 51,
};

const unsigned axis1_wave26_mesh_unsigned_offsets[38] = {
    26u, 30u, 34u, 38u, 42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u, 74u, 78u, 82u, 86u,
    90u, 94u, 98u, 102u, 106u, 110u, 114u, 118u, 122u, 126u, 130u, 134u, 138u, 142u, 146u, 150u,
    154u, 158u, 162u, 166u, 170u, 174u,
};

const unsigned* axis1_wave26_mesh_windows[41] = {
    axis1_wave26_mesh_weights + 0, axis1_wave26_mesh_weights + 2,
    axis1_wave26_mesh_weights + 4, axis1_wave26_mesh_weights + 6,
    axis1_wave26_mesh_weights + 8, axis1_wave26_mesh_weights + 10,
    axis1_wave26_mesh_weights + 12, axis1_wave26_mesh_weights + 14,
    axis1_wave26_mesh_weights + 16, axis1_wave26_mesh_weights + 18,
    axis1_wave26_mesh_weights + 20, axis1_wave26_mesh_weights + 22,
    axis1_wave26_mesh_weights + 24, axis1_wave26_mesh_weights + 26,
    axis1_wave26_mesh_weights + 28, axis1_wave26_mesh_weights + 30,
    axis1_wave26_mesh_weights + 32, axis1_wave26_mesh_weights + 34,
    axis1_wave26_mesh_weights + 1, axis1_wave26_mesh_weights + 3,
    axis1_wave26_mesh_weights + 5, axis1_wave26_mesh_weights + 7,
    axis1_wave26_mesh_weights + 9, axis1_wave26_mesh_weights + 11,
    axis1_wave26_mesh_weights + 13, axis1_wave26_mesh_weights + 15,
    axis1_wave26_mesh_weights + 17, axis1_wave26_mesh_weights + 19,
    axis1_wave26_mesh_weights + 21, axis1_wave26_mesh_weights + 23,
    axis1_wave26_mesh_weights + 25, axis1_wave26_mesh_weights + 27,
    axis1_wave26_mesh_weights + 29, axis1_wave26_mesh_weights + 31,
    axis1_wave26_mesh_weights + 33, axis1_wave26_mesh_weights + 35,
    axis1_wave26_mesh_weights + 37, axis1_wave26_mesh_weights + 39,
    axis1_wave26_mesh_weights + 41, axis1_wave26_mesh_weights + 43,
    axis1_wave26_mesh_weights + 45,
};

const unsigned** axis1_wave26_mesh_routes[16] = {
    axis1_wave26_mesh_windows + 0, axis1_wave26_mesh_windows + 2,
    axis1_wave26_mesh_windows + 4, axis1_wave26_mesh_windows + 6,
    axis1_wave26_mesh_windows + 8, axis1_wave26_mesh_windows + 10,
    axis1_wave26_mesh_windows + 12, axis1_wave26_mesh_windows + 14,
    axis1_wave26_mesh_windows + 16, axis1_wave26_mesh_windows + 18,
    axis1_wave26_mesh_windows + 20, axis1_wave26_mesh_windows + 22,
    axis1_wave26_mesh_windows + 24, axis1_wave26_mesh_windows + 26,
    axis1_wave26_mesh_windows + 28, axis1_wave26_mesh_windows + 30,
};

const unsigned*** axis1_wave26_mesh_plans[3] = {
    axis1_wave26_mesh_routes + 0,
    axis1_wave26_mesh_routes + 5,
    axis1_wave26_mesh_routes + 10,
};
