const unsigned axis1_wave23_reseed_weights[70] = {
    24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u, 60u, 64u, 68u,
    72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u, 108u, 112u, 116u,
    120u, 124u, 128u, 132u, 136u, 140u, 144u, 148u, 152u, 156u, 160u, 164u,
    168u, 172u, 176u, 180u, 184u, 188u, 192u, 196u, 200u, 204u, 208u, 212u,
    216u, 220u, 224u, 228u, 232u, 236u, 240u, 244u, 248u, 252u, 256u, 260u,
    264u, 268u, 272u, 276u, 280u, 284u, 288u, 292u, 296u, 300u,
};

const unsigned axis1_wave23_reseed_lane_masks[18] = {
    5u, 9u, 6u, 10u, 7u, 11u, 8u, 12u, 13u, 14u, 15u, 16u, 17u, 19u, 21u, 23u, 25u, 27u,
};

const int axis1_wave23_reseed_signed_offsets[32] = {
    -18, 13, -15, 20, -12, 23, -17, 14, -10, 26, -16, 15, -13, 28, -11, 30,
    -14, 22, -9, 32, -19, 17, -8, 34, -7, 36, -6, 38, -5, 40, -4, 42,
};

const unsigned axis1_wave23_reseed_unsigned_offsets[32] = {
    20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u, 56u, 60u, 64u, 68u, 72u, 76u, 80u,
    84u, 88u, 92u, 96u, 100u, 104u, 108u, 112u, 116u, 120u, 124u, 128u, 132u, 136u, 140u, 144u,
};

const unsigned* axis1_wave23_reseed_windows[35] = {
    axis1_wave23_reseed_weights + 0, axis1_wave23_reseed_weights + 2,
    axis1_wave23_reseed_weights + 4, axis1_wave23_reseed_weights + 6,
    axis1_wave23_reseed_weights + 8, axis1_wave23_reseed_weights + 10,
    axis1_wave23_reseed_weights + 12, axis1_wave23_reseed_weights + 14,
    axis1_wave23_reseed_weights + 16, axis1_wave23_reseed_weights + 18,
    axis1_wave23_reseed_weights + 20, axis1_wave23_reseed_weights + 22,
    axis1_wave23_reseed_weights + 24, axis1_wave23_reseed_weights + 26,
    axis1_wave23_reseed_weights + 28, axis1_wave23_reseed_weights + 30,
    axis1_wave23_reseed_weights + 32, axis1_wave23_reseed_weights + 34,
    axis1_wave23_reseed_weights + 1, axis1_wave23_reseed_weights + 3,
    axis1_wave23_reseed_weights + 5, axis1_wave23_reseed_weights + 7,
    axis1_wave23_reseed_weights + 9, axis1_wave23_reseed_weights + 11,
    axis1_wave23_reseed_weights + 13, axis1_wave23_reseed_weights + 15,
    axis1_wave23_reseed_weights + 17, axis1_wave23_reseed_weights + 19,
    axis1_wave23_reseed_weights + 21, axis1_wave23_reseed_weights + 23,
    axis1_wave23_reseed_weights + 25, axis1_wave23_reseed_weights + 27,
    axis1_wave23_reseed_weights + 29, axis1_wave23_reseed_weights + 31,
    axis1_wave23_reseed_weights + 33,
};

const unsigned** axis1_wave23_reseed_routes[16] = {
    axis1_wave23_reseed_windows + 0, axis1_wave23_reseed_windows + 2,
    axis1_wave23_reseed_windows + 4, axis1_wave23_reseed_windows + 6,
    axis1_wave23_reseed_windows + 8, axis1_wave23_reseed_windows + 10,
    axis1_wave23_reseed_windows + 12, axis1_wave23_reseed_windows + 14,
    axis1_wave23_reseed_windows + 16, axis1_wave23_reseed_windows + 18,
    axis1_wave23_reseed_windows + 20, axis1_wave23_reseed_windows + 22,
    axis1_wave23_reseed_windows + 24, axis1_wave23_reseed_windows + 26,
    axis1_wave23_reseed_windows + 28, axis1_wave23_reseed_windows + 30,
};

const unsigned*** axis1_wave23_reseed_plans[3] = {
    axis1_wave23_reseed_routes + 0,
    axis1_wave23_reseed_routes + 5,
    axis1_wave23_reseed_routes + 10,
};
