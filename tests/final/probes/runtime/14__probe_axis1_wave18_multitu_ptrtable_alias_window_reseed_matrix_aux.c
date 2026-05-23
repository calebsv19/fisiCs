extern const unsigned axis1_wave18_window_weights[50];
extern const unsigned axis1_wave18_window_lane_masks[12];
extern const int axis1_wave18_window_signed_offsets[22];
extern const unsigned axis1_wave18_window_unsigned_offsets[22];
extern const unsigned* axis1_wave18_window_windows[25];
extern const unsigned** axis1_wave18_window_routes[12];
extern const unsigned*** axis1_wave18_window_plans[3];

unsigned axis1_wave18_ptrtable_alias_window_reseed_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave18_window_weights[(seed + 13u) % 50u];
    unsigned reseed = (seed % 7u) + 2u;
    unsigned lane = 0u;

    (void)axis1_wave18_window_windows;
    (void)axis1_wave18_window_routes;

    for (; lane < 12u; ++lane) {
        const unsigned*** plan = axis1_wave18_window_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + reseed) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + reseed + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave18_window_lane_masks[lane];
        int soff = axis1_wave18_window_signed_offsets[(lane + reseed + mask) % 22u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave18_window_unsigned_offsets[(lane * 2u + reseed + mask) % 22u];

        acc = ((acc ^ base[0]) + alias[1] + skew + reseed) * 39u;
        acc ^= axis1_wave18_window_weights[(lane + uoff + base[1]) % 50u];
        acc += axis1_wave18_window_weights[(lane * 3u + alias[0] + 7u) % 50u];
        reseed = (reseed + (base[1] & 3u) + (alias[0] & 1u)) % 13u;
    }

    return acc ^ axis1_wave18_window_weights[(seed + reseed + 25u) % 50u];
}
