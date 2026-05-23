const unsigned axis1_wave22_weave_weights[64] = {
    20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u, 60u, 64u,
    68u, 72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u, 108u, 112u,
    116u, 120u, 124u, 128u, 132u, 136u, 140u, 144u, 148u, 152u, 156u, 160u,
    164u, 168u, 172u, 176u, 180u, 184u, 188u, 192u, 196u, 200u, 204u, 208u,
    212u, 216u, 220u, 224u, 228u, 232u, 236u, 240u, 244u, 248u, 252u, 256u,
    260u, 264u, 268u, 272u,
};

const int axis1_wave22_weave_signed_offsets[32] = {
    -18, 12, -15, 18, -12, 20, -16, 13, -10, 22, -17, 14, -13, 24, -11, 26,
    -14, 21, -9, 27, -19, 16, -8, 28, -7, 30, -6, 32, -5, 33, -4, 35,
};

const unsigned axis1_wave22_weave_unsigned_offsets[32] = {
    17u, 21u, 25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u,
    81u, 85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u, 125u, 129u, 133u, 137u, 141u,
};

const unsigned* axis1_wave22_weave_windows[32] = {
    axis1_wave22_weave_weights + 0, axis1_wave22_weave_weights + 2,
    axis1_wave22_weave_weights + 4, axis1_wave22_weave_weights + 6,
    axis1_wave22_weave_weights + 8, axis1_wave22_weave_weights + 10,
    axis1_wave22_weave_weights + 12, axis1_wave22_weave_weights + 14,
    axis1_wave22_weave_weights + 16, axis1_wave22_weave_weights + 18,
    axis1_wave22_weave_weights + 20, axis1_wave22_weave_weights + 22,
    axis1_wave22_weave_weights + 24, axis1_wave22_weave_weights + 26,
    axis1_wave22_weave_weights + 28, axis1_wave22_weave_weights + 30,
    axis1_wave22_weave_weights + 32, axis1_wave22_weave_weights + 34,
    axis1_wave22_weave_weights + 36, axis1_wave22_weave_weights + 38,
    axis1_wave22_weave_weights + 40, axis1_wave22_weave_weights + 42,
    axis1_wave22_weave_weights + 44, axis1_wave22_weave_weights + 46,
    axis1_wave22_weave_weights + 48, axis1_wave22_weave_weights + 50,
    axis1_wave22_weave_weights + 52, axis1_wave22_weave_weights + 54,
    axis1_wave22_weave_weights + 56, axis1_wave22_weave_weights + 58,
    axis1_wave22_weave_weights + 60, axis1_wave22_weave_weights + 62,
};

const unsigned** axis1_wave22_weave_routes[16] = {
    axis1_wave22_weave_windows + 0, axis1_wave22_weave_windows + 2,
    axis1_wave22_weave_windows + 4, axis1_wave22_weave_windows + 6,
    axis1_wave22_weave_windows + 8, axis1_wave22_weave_windows + 10,
    axis1_wave22_weave_windows + 12, axis1_wave22_weave_windows + 14,
    axis1_wave22_weave_windows + 1, axis1_wave22_weave_windows + 3,
    axis1_wave22_weave_windows + 5, axis1_wave22_weave_windows + 7,
    axis1_wave22_weave_windows + 9, axis1_wave22_weave_windows + 11,
    axis1_wave22_weave_windows + 13, axis1_wave22_weave_windows + 15,
};

const unsigned*** axis1_wave22_weave_plans[4] = {
    axis1_wave22_weave_routes + 0,
    axis1_wave22_weave_routes + 4,
    axis1_wave22_weave_routes + 8,
    axis1_wave22_weave_routes + 12,
};
