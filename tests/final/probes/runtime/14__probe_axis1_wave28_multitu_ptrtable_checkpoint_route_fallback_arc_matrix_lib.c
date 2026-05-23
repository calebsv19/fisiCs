const unsigned axis1_wave28_fallback_arc_weights[88] = {
    37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u, 77u, 81u,
    85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u, 125u, 129u,
    133u, 137u, 141u, 145u, 149u, 153u, 157u, 161u, 165u, 169u, 173u, 177u,
    181u, 185u, 189u, 193u, 197u, 201u, 205u, 209u, 213u, 217u, 221u, 225u,
    229u, 233u, 237u, 241u, 245u, 249u, 253u, 257u, 261u, 265u, 269u, 273u,
    277u, 281u, 285u, 289u, 293u, 297u, 301u, 305u, 309u, 313u, 317u, 321u,
    325u, 329u, 333u, 337u, 341u, 345u, 349u, 353u, 357u, 361u, 365u, 369u,
    373u, 377u, 381u, 385u
};

const int axis1_wave28_fallback_arc_signed_offsets[44] = {
    -24, 19, -21, 26, -18, 29, -23, 20, -16, 32, -22, 21, -19, 34, -17, 36,
    -20, 28, -15, 38, -25, 23, -14, 40, -13, 42, -12, 44, -11, 46, -10, 48,
    -9, 50, -8, 52, -7, 54, -6, 56, -5, 58, -4, 60
};

const unsigned axis1_wave28_fallback_arc_unsigned_offsets[44] = {
    29u, 33u, 37u, 41u, 45u, 49u, 53u, 57u, 61u, 65u, 69u, 73u,
    77u, 81u, 85u, 89u, 93u, 97u, 101u, 105u, 109u, 113u, 117u, 121u,
    125u, 129u, 133u, 137u, 141u, 145u, 149u, 153u, 157u, 161u, 165u, 169u,
    173u, 177u, 181u, 185u, 189u, 193u, 197u, 201u
};

const unsigned* axis1_wave28_fallback_arc_windows[44] = {
    axis1_wave28_fallback_arc_weights + 0, axis1_wave28_fallback_arc_weights + 2,
    axis1_wave28_fallback_arc_weights + 4, axis1_wave28_fallback_arc_weights + 6,
    axis1_wave28_fallback_arc_weights + 8, axis1_wave28_fallback_arc_weights + 10,
    axis1_wave28_fallback_arc_weights + 12, axis1_wave28_fallback_arc_weights + 14,
    axis1_wave28_fallback_arc_weights + 16, axis1_wave28_fallback_arc_weights + 18,
    axis1_wave28_fallback_arc_weights + 20, axis1_wave28_fallback_arc_weights + 22,
    axis1_wave28_fallback_arc_weights + 24, axis1_wave28_fallback_arc_weights + 26,
    axis1_wave28_fallback_arc_weights + 28, axis1_wave28_fallback_arc_weights + 30,
    axis1_wave28_fallback_arc_weights + 32, axis1_wave28_fallback_arc_weights + 34,
    axis1_wave28_fallback_arc_weights + 1, axis1_wave28_fallback_arc_weights + 3,
    axis1_wave28_fallback_arc_weights + 5, axis1_wave28_fallback_arc_weights + 7,
    axis1_wave28_fallback_arc_weights + 9, axis1_wave28_fallback_arc_weights + 11,
    axis1_wave28_fallback_arc_weights + 13, axis1_wave28_fallback_arc_weights + 15,
    axis1_wave28_fallback_arc_weights + 17, axis1_wave28_fallback_arc_weights + 19,
    axis1_wave28_fallback_arc_weights + 21, axis1_wave28_fallback_arc_weights + 23,
    axis1_wave28_fallback_arc_weights + 25, axis1_wave28_fallback_arc_weights + 27,
    axis1_wave28_fallback_arc_weights + 29, axis1_wave28_fallback_arc_weights + 31,
    axis1_wave28_fallback_arc_weights + 33, axis1_wave28_fallback_arc_weights + 35,
    axis1_wave28_fallback_arc_weights + 37, axis1_wave28_fallback_arc_weights + 39,
    axis1_wave28_fallback_arc_weights + 41, axis1_wave28_fallback_arc_weights + 43,
    axis1_wave28_fallback_arc_weights + 45, axis1_wave28_fallback_arc_weights + 47,
    axis1_wave28_fallback_arc_weights + 49, axis1_wave28_fallback_arc_weights + 51
};

const unsigned** axis1_wave28_fallback_arc_routes[16] = {
    axis1_wave28_fallback_arc_windows + 0, axis1_wave28_fallback_arc_windows + 2,
    axis1_wave28_fallback_arc_windows + 4, axis1_wave28_fallback_arc_windows + 6,
    axis1_wave28_fallback_arc_windows + 8, axis1_wave28_fallback_arc_windows + 10,
    axis1_wave28_fallback_arc_windows + 12, axis1_wave28_fallback_arc_windows + 14,
    axis1_wave28_fallback_arc_windows + 16, axis1_wave28_fallback_arc_windows + 18,
    axis1_wave28_fallback_arc_windows + 20, axis1_wave28_fallback_arc_windows + 22,
    axis1_wave28_fallback_arc_windows + 24, axis1_wave28_fallback_arc_windows + 26,
    axis1_wave28_fallback_arc_windows + 28, axis1_wave28_fallback_arc_windows + 30
};

const unsigned*** axis1_wave28_fallback_arc_plans[4] = {
    axis1_wave28_fallback_arc_routes + 0,
    axis1_wave28_fallback_arc_routes + 4,
    axis1_wave28_fallback_arc_routes + 8,
    axis1_wave28_fallback_arc_routes + 12
};
