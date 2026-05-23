extern const unsigned axis1_wave25_braid_weights[78];
extern const unsigned axis1_wave25_braid_lane_masks[20];
extern const int axis1_wave25_braid_signed_offsets[36];
extern const unsigned axis1_wave25_braid_unsigned_offsets[36];
extern const unsigned* axis1_wave25_braid_windows[39];
extern const unsigned** axis1_wave25_braid_routes[16];
extern const unsigned*** axis1_wave25_braid_plans[3];

unsigned axis1_wave25_ptrtable_alias_window_replay_braid_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave25_braid_weights[(seed + 23u) % 78u];
    unsigned replay = (seed % 19u) + 2u;
    unsigned braid = (seed % 15u) + 3u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave25_braid_plans[(lane + (seed & 1u) + (braid & 1u)) % 3u];
        const unsigned** route = plan[(lane + replay + braid) % 5u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + replay + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave25_braid_lane_masks[(lane + braid) % 20u];
        int soff = axis1_wave25_braid_signed_offsets[(lane + replay + mask) % 36u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave25_braid_unsigned_offsets[(lane * 2u + replay + braid + mask) % 36u];

        acc = ((acc ^ base[0]) + alias[1] + skew + replay) * 101u;
        acc ^= axis1_wave25_braid_weights[(lane + uoff + base[1]) % 78u];
        acc += axis1_wave25_braid_weights[(lane * 3u + alias[0] + 17u) % 78u];
        replay = (replay + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 33u;
        braid = (braid + replay + (base[0] & 7u)) % 39u;
    }

    return acc ^ axis1_wave25_braid_weights[(seed + replay + braid + 39u) % 78u];
}
