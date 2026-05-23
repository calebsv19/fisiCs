const unsigned axis1_wave26_lattice_weights[80] = {
    32u, 36u, 40u, 44u, 48u, 52u, 56u, 60u, 64u, 68u, 72u, 76u,
    80u, 84u, 88u, 92u, 96u, 100u, 104u, 108u, 112u, 116u, 120u, 124u,
    128u, 132u, 136u, 140u, 144u, 148u, 152u, 156u, 160u, 164u, 168u, 172u,
    176u, 180u, 184u, 188u, 192u, 196u, 200u, 204u, 208u, 212u, 216u, 220u,
    224u, 228u, 232u, 236u, 240u, 244u, 248u, 252u, 256u, 260u, 264u, 268u,
    272u, 276u, 280u, 284u, 288u, 292u, 296u, 300u, 304u, 308u, 312u, 316u,
    320u, 324u, 328u, 332u, 336u, 340u, 344u, 348u,
};

const int axis1_wave26_lattice_signed_offsets[40] = {
    -22, 17, -19, 24, -16, 27, -21, 18, -14, 30, -20, 19, -17, 32, -15, 34,
    -18, 26, -13, 36, -23, 21, -12, 38, -11, 40, -10, 42, -9, 44, -8, 46, -7, 48, -6, 50, -5, 52, -4, 54,
};

const unsigned axis1_wave26_lattice_unsigned_offsets[40] = {
    25u, 29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u, 85u,
    89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u, 125u, 129u, 133u, 137u, 141u, 145u, 149u,
    153u, 157u, 161u, 165u, 169u, 173u, 177u, 181u,
};

const unsigned* axis1_wave26_lattice_windows[40] = {
    axis1_wave26_lattice_weights + 0, axis1_wave26_lattice_weights + 2,
    axis1_wave26_lattice_weights + 4, axis1_wave26_lattice_weights + 6,
    axis1_wave26_lattice_weights + 8, axis1_wave26_lattice_weights + 10,
    axis1_wave26_lattice_weights + 12, axis1_wave26_lattice_weights + 14,
    axis1_wave26_lattice_weights + 16, axis1_wave26_lattice_weights + 18,
    axis1_wave26_lattice_weights + 20, axis1_wave26_lattice_weights + 22,
    axis1_wave26_lattice_weights + 24, axis1_wave26_lattice_weights + 26,
    axis1_wave26_lattice_weights + 28, axis1_wave26_lattice_weights + 30,
    axis1_wave26_lattice_weights + 32, axis1_wave26_lattice_weights + 34,
    axis1_wave26_lattice_weights + 1, axis1_wave26_lattice_weights + 3,
    axis1_wave26_lattice_weights + 5, axis1_wave26_lattice_weights + 7,
    axis1_wave26_lattice_weights + 9, axis1_wave26_lattice_weights + 11,
    axis1_wave26_lattice_weights + 13, axis1_wave26_lattice_weights + 15,
    axis1_wave26_lattice_weights + 17, axis1_wave26_lattice_weights + 19,
    axis1_wave26_lattice_weights + 21, axis1_wave26_lattice_weights + 23,
    axis1_wave26_lattice_weights + 25, axis1_wave26_lattice_weights + 27,
    axis1_wave26_lattice_weights + 29, axis1_wave26_lattice_weights + 31,
    axis1_wave26_lattice_weights + 33, axis1_wave26_lattice_weights + 35,
    axis1_wave26_lattice_weights + 37, axis1_wave26_lattice_weights + 39,
};

const unsigned** axis1_wave26_lattice_routes[16] = {
    axis1_wave26_lattice_windows + 0, axis1_wave26_lattice_windows + 2,
    axis1_wave26_lattice_windows + 4, axis1_wave26_lattice_windows + 6,
    axis1_wave26_lattice_windows + 8, axis1_wave26_lattice_windows + 10,
    axis1_wave26_lattice_windows + 12, axis1_wave26_lattice_windows + 14,
    axis1_wave26_lattice_windows + 16, axis1_wave26_lattice_windows + 18,
    axis1_wave26_lattice_windows + 20, axis1_wave26_lattice_windows + 22,
    axis1_wave26_lattice_windows + 24, axis1_wave26_lattice_windows + 26,
    axis1_wave26_lattice_windows + 28, axis1_wave26_lattice_windows + 30,
};

const unsigned*** axis1_wave26_lattice_plans[4] = {
    axis1_wave26_lattice_routes + 0,
    axis1_wave26_lattice_routes + 4,
    axis1_wave26_lattice_routes + 8,
    axis1_wave26_lattice_routes + 12,
};
