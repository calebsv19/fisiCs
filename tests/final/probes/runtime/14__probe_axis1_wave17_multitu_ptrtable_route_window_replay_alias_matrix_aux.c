extern const unsigned axis1_wave17_replay_weights[46];
extern const unsigned axis1_wave17_replay_lane_masks[12];
extern const int axis1_wave17_replay_signed_offsets[20];
extern const unsigned axis1_wave17_replay_unsigned_offsets[20];
extern const unsigned* axis1_wave17_replay_windows[23];
extern const unsigned** axis1_wave17_replay_routes[12];
extern const unsigned*** axis1_wave17_replay_plans[3];

unsigned axis1_wave17_ptrtable_route_window_replay_alias_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave17_replay_weights[(seed + 7u) % 46u];
    unsigned replay = (seed % 5u) + 1u;
    unsigned lane = 0u;

    (void)axis1_wave17_replay_windows;
    (void)axis1_wave17_replay_routes;

    for (; lane < 12u; ++lane) {
        const unsigned*** plan = axis1_wave17_replay_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + replay) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* shadow = route[((lane + replay + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave17_replay_lane_masks[lane];
        int soff = axis1_wave17_replay_signed_offsets[(lane + replay + mask) % 20u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave17_replay_unsigned_offsets[(lane * 2u + replay + mask) % 20u];
        unsigned alias_pick = ((mask ^ replay ^ uoff) & 1u);
        const unsigned* chosen = alias_pick ? shadow : base;
        const unsigned* alias = alias_pick ? base : shadow;

        acc = ((acc ^ chosen[0]) + alias[1] + skew + replay) * 35u;
        acc ^= axis1_wave17_replay_weights[(lane + uoff + chosen[1]) % 46u];
        acc += axis1_wave17_replay_weights[(lane * 3u + alias[0] + 5u) % 46u];
        replay = (replay + (chosen[1] & 3u) + (alias[0] & 1u)) % 11u;
    }

    return acc ^ axis1_wave17_replay_weights[(seed + replay + 21u) % 46u];
}
