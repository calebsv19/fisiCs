const unsigned axis1_wave25_mesh_weights[76] = {
    29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u,
    77u, 81u, 85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u,
    125u, 129u, 133u, 137u, 141u, 145u, 149u, 153u, 157u, 161u, 165u, 169u,
    173u, 177u, 181u, 185u, 189u, 193u, 197u, 201u, 205u, 209u, 213u, 217u,
    221u, 225u, 229u, 233u, 237u, 241u, 245u, 249u, 253u, 257u, 261u, 265u,
    269u, 273u, 277u, 281u, 285u, 289u, 293u, 297u, 301u, 305u, 309u, 313u,
    317u, 321u, 325u, 329u,
};

const int axis1_wave25_mesh_signed_offsets[38] = {
    -21, 16, -18, 23, -15, 26, -20, 17, -13, 29, -19, 18, -16, 31, -14, 33,
    -17, 25, -12, 35, -22, 20, -11, 37, -10, 39, -9, 41, -8, 43, -7, 45, -6, 47, -5, 49, -4, 51,
};

const unsigned axis1_wave25_mesh_unsigned_offsets[38] = {
    23u, 27u, 31u, 35u, 39u, 43u, 47u, 51u, 55u, 59u, 63u, 67u, 71u, 75u, 79u, 83u,
    87u, 91u, 95u, 99u, 103u, 107u, 111u, 115u, 119u, 123u, 127u, 131u, 135u, 139u, 143u, 147u,
    151u, 155u, 159u, 163u, 167u, 171u,
};

const unsigned* axis1_wave25_mesh_windows[38] = {
    axis1_wave25_mesh_weights + 0, axis1_wave25_mesh_weights + 2,
    axis1_wave25_mesh_weights + 4, axis1_wave25_mesh_weights + 6,
    axis1_wave25_mesh_weights + 8, axis1_wave25_mesh_weights + 10,
    axis1_wave25_mesh_weights + 12, axis1_wave25_mesh_weights + 14,
    axis1_wave25_mesh_weights + 16, axis1_wave25_mesh_weights + 18,
    axis1_wave25_mesh_weights + 20, axis1_wave25_mesh_weights + 22,
    axis1_wave25_mesh_weights + 24, axis1_wave25_mesh_weights + 26,
    axis1_wave25_mesh_weights + 28, axis1_wave25_mesh_weights + 30,
    axis1_wave25_mesh_weights + 32, axis1_wave25_mesh_weights + 34,
    axis1_wave25_mesh_weights + 1, axis1_wave25_mesh_weights + 3,
    axis1_wave25_mesh_weights + 5, axis1_wave25_mesh_weights + 7,
    axis1_wave25_mesh_weights + 9, axis1_wave25_mesh_weights + 11,
    axis1_wave25_mesh_weights + 13, axis1_wave25_mesh_weights + 15,
    axis1_wave25_mesh_weights + 17, axis1_wave25_mesh_weights + 19,
    axis1_wave25_mesh_weights + 21, axis1_wave25_mesh_weights + 23,
    axis1_wave25_mesh_weights + 25, axis1_wave25_mesh_weights + 27,
    axis1_wave25_mesh_weights + 29, axis1_wave25_mesh_weights + 31,
    axis1_wave25_mesh_weights + 33, axis1_wave25_mesh_weights + 35,
    axis1_wave25_mesh_weights + 37, axis1_wave25_mesh_weights + 39,
};

const unsigned** axis1_wave25_mesh_routes[16] = {
    axis1_wave25_mesh_windows + 0, axis1_wave25_mesh_windows + 2,
    axis1_wave25_mesh_windows + 4, axis1_wave25_mesh_windows + 6,
    axis1_wave25_mesh_windows + 8, axis1_wave25_mesh_windows + 10,
    axis1_wave25_mesh_windows + 12, axis1_wave25_mesh_windows + 14,
    axis1_wave25_mesh_windows + 16, axis1_wave25_mesh_windows + 18,
    axis1_wave25_mesh_windows + 20, axis1_wave25_mesh_windows + 22,
    axis1_wave25_mesh_windows + 24, axis1_wave25_mesh_windows + 26,
    axis1_wave25_mesh_windows + 28, axis1_wave25_mesh_windows + 30,
};

const unsigned*** axis1_wave25_mesh_plans[4] = {
    axis1_wave25_mesh_routes + 0,
    axis1_wave25_mesh_routes + 4,
    axis1_wave25_mesh_routes + 8,
    axis1_wave25_mesh_routes + 12,
};
