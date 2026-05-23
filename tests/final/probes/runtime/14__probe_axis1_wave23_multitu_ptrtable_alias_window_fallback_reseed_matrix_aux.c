extern const unsigned axis1_wave23_reseed_weights[70];
extern const unsigned axis1_wave23_reseed_lane_masks[18];
extern const int axis1_wave23_reseed_signed_offsets[32];
extern const unsigned axis1_wave23_reseed_unsigned_offsets[32];
extern const unsigned* axis1_wave23_reseed_windows[35];
extern const unsigned** axis1_wave23_reseed_routes[16];
extern const unsigned*** axis1_wave23_reseed_plans[3];

unsigned axis1_wave23_ptrtable_alias_window_fallback_reseed_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave23_reseed_weights[(seed + 19u) % 70u];
    unsigned replay = (seed % 15u) + 2u;
    unsigned fallback = (seed % 11u) + 4u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave23_reseed_plans[(lane + (seed & 1u) + (fallback & 1u)) % 3u];
        const unsigned** route = plan[(lane + replay + fallback) % 5u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + replay + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave23_reseed_lane_masks[(lane + fallback) % 18u];
        int soff = axis1_wave23_reseed_signed_offsets[(lane + replay + mask) % 32u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave23_reseed_unsigned_offsets[(lane * 2u + replay + fallback + mask) % 32u];

        acc = ((acc ^ base[0]) + alias[1] + skew + fallback) * 79u;
        acc ^= axis1_wave23_reseed_weights[(lane + uoff + base[1]) % 70u];
        acc += axis1_wave23_reseed_weights[(lane * 3u + alias[0] + 13u) % 70u];
        replay = (replay + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 29u;
        fallback = (fallback + replay + (base[0] & 7u)) % 33u;
    }

    return acc ^ axis1_wave23_reseed_weights[(seed + replay + fallback + 35u) % 70u];
}
