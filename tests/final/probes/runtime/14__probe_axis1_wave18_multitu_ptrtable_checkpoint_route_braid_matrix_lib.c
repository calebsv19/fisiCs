const unsigned axis1_wave18_braid_weights[48] = {
    12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u,
    60u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u,
    108u, 112u, 116u, 120u, 124u, 128u, 132u, 136u, 140u, 144u, 148u, 152u,
    156u, 160u, 164u, 168u, 172u, 176u, 180u, 184u, 188u, 192u, 196u, 200u,
};

const int axis1_wave18_braid_signed_offsets[24] = {
    -14, 7, -11, 13, -8, 15, -12, 9, -6, 17, -13, 10,
    -9, 19, -7, 21, -10, 16, -5, 22, -15, 12, -4, 23,
};

const unsigned axis1_wave18_braid_unsigned_offsets[24] = {
    9u, 13u, 17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u,
    57u, 61u, 65u, 69u, 73u, 77u, 81u, 85u, 89u, 93u, 97u, 101u,
};

const unsigned* axis1_wave18_braid_windows[24] = {
    axis1_wave18_braid_weights + 0, axis1_wave18_braid_weights + 2,
    axis1_wave18_braid_weights + 4, axis1_wave18_braid_weights + 6,
    axis1_wave18_braid_weights + 8, axis1_wave18_braid_weights + 10,
    axis1_wave18_braid_weights + 12, axis1_wave18_braid_weights + 14,
    axis1_wave18_braid_weights + 16, axis1_wave18_braid_weights + 18,
    axis1_wave18_braid_weights + 20, axis1_wave18_braid_weights + 22,
    axis1_wave18_braid_weights + 24, axis1_wave18_braid_weights + 26,
    axis1_wave18_braid_weights + 28, axis1_wave18_braid_weights + 30,
    axis1_wave18_braid_weights + 32, axis1_wave18_braid_weights + 34,
    axis1_wave18_braid_weights + 36, axis1_wave18_braid_weights + 38,
    axis1_wave18_braid_weights + 40, axis1_wave18_braid_weights + 42,
    axis1_wave18_braid_weights + 44, axis1_wave18_braid_weights + 46,
};

const unsigned** axis1_wave18_braid_routes[12] = {
    axis1_wave18_braid_windows + 0, axis1_wave18_braid_windows + 2,
    axis1_wave18_braid_windows + 4, axis1_wave18_braid_windows + 6,
    axis1_wave18_braid_windows + 8, axis1_wave18_braid_windows + 10,
    axis1_wave18_braid_windows + 1, axis1_wave18_braid_windows + 3,
    axis1_wave18_braid_windows + 5, axis1_wave18_braid_windows + 7,
    axis1_wave18_braid_windows + 9, axis1_wave18_braid_windows + 11,
};

const unsigned*** axis1_wave18_braid_plans[4] = {
    axis1_wave18_braid_routes + 0,
    axis1_wave18_braid_routes + 3,
    axis1_wave18_braid_routes + 6,
    axis1_wave18_braid_routes + 8,
};
