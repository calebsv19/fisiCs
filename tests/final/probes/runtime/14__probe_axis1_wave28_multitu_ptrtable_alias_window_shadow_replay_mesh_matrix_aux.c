extern const unsigned axis1_wave28_shadow_replay_weights[90];
extern const unsigned axis1_wave28_shadow_replay_lane_masks[24];
extern const int axis1_wave28_shadow_replay_signed_offsets[42];
extern const unsigned axis1_wave28_shadow_replay_unsigned_offsets[42];
extern const unsigned* axis1_wave28_shadow_replay_windows[45];
extern const unsigned** axis1_wave28_shadow_replay_routes[16];
extern const unsigned*** axis1_wave28_shadow_replay_plans[3];

unsigned axis1_wave28_ptrtable_alias_window_shadow_replay_mesh_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave28_shadow_replay_weights[(seed + 29u) % 90u];
    unsigned replay = (seed % 25u) + 2u;
    unsigned shadow = (seed % 21u) + 4u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave28_shadow_replay_plans[(lane + (seed & 1u) + (shadow & 1u)) % 3u];
        const unsigned** route = plan[(lane + replay + shadow) % 5u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + replay + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave28_shadow_replay_lane_masks[(lane + shadow) % 24u];
        int soff = axis1_wave28_shadow_replay_signed_offsets[(lane + replay + mask) % 42u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave28_shadow_replay_unsigned_offsets[(lane * 2u + replay + shadow + mask) % 42u];

        acc = ((acc ^ base[0]) + alias[1] + skew + replay) * 131u;
        acc ^= axis1_wave28_shadow_replay_weights[(lane + uoff + base[1]) % 90u];
        acc += axis1_wave28_shadow_replay_weights[(lane * 3u + alias[0] + 25u) % 90u];
        replay = (replay + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 39u;
        shadow = (shadow + replay + (base[0] & 7u)) % 47u;
    }

    return acc ^ axis1_wave28_shadow_replay_weights[(seed + replay + shadow + 49u) % 90u];
}
