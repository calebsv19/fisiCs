extern const unsigned axis1_wave19_window_weights[54];
extern const unsigned axis1_wave19_window_lane_masks[13];
extern const int axis1_wave19_window_signed_offsets[24];
extern const unsigned axis1_wave19_window_unsigned_offsets[24];
extern const unsigned* axis1_wave19_window_windows[27];
extern const unsigned** axis1_wave19_window_routes[13];
extern const unsigned*** axis1_wave19_window_plans[3];

unsigned axis1_wave19_ptrtable_alias_window_replay_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave19_window_weights[(seed + 7u) % 54u];
    unsigned replay = (seed % 9u) + 2u;
    unsigned lane = 0u;

    (void)axis1_wave19_window_windows;
    (void)axis1_wave19_window_routes;

    for (; lane < 13u; ++lane) {
        const unsigned*** plan = axis1_wave19_window_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + replay) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + replay + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave19_window_lane_masks[lane];
        int soff = axis1_wave19_window_signed_offsets[(lane + replay + mask) % 24u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave19_window_unsigned_offsets[(lane * 2u + replay + mask) % 24u];

        acc = ((acc ^ base[0]) + alias[1] + skew + replay) * 43u;
        acc ^= axis1_wave19_window_weights[(lane + uoff + base[1]) % 54u];
        acc += axis1_wave19_window_weights[(lane * 3u + alias[0] + 9u) % 54u];
        replay = (replay + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 17u;
    }

    return acc ^ axis1_wave19_window_weights[(seed + replay + 27u) % 54u];
}
