extern const unsigned axis1_wave24_braid_weights[74];
extern const unsigned axis1_wave24_braid_lane_masks[20];
extern const int axis1_wave24_braid_signed_offsets[34];
extern const unsigned axis1_wave24_braid_unsigned_offsets[34];
extern const unsigned* axis1_wave24_braid_windows[37];
extern const unsigned** axis1_wave24_braid_routes[16];
extern const unsigned*** axis1_wave24_braid_plans[3];

unsigned axis1_wave24_ptrtable_alias_window_fallback_braid_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave24_braid_weights[(seed + 21u) % 74u];
    unsigned fallback = (seed % 17u) + 2u;
    unsigned braid = (seed % 13u) + 4u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave24_braid_plans[(lane + (seed & 1u) + (braid & 1u)) % 3u];
        const unsigned** route = plan[(lane + fallback + braid) % 5u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + fallback + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave24_braid_lane_masks[(lane + braid) % 20u];
        int soff = axis1_wave24_braid_signed_offsets[(lane + fallback + mask) % 34u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave24_braid_unsigned_offsets[(lane * 2u + fallback + braid + mask) % 34u];

        acc = ((acc ^ base[0]) + alias[1] + skew + fallback) * 89u;
        acc ^= axis1_wave24_braid_weights[(lane + uoff + base[1]) % 74u];
        acc += axis1_wave24_braid_weights[(lane * 3u + alias[0] + 15u) % 74u];
        fallback = (fallback + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 31u;
        braid = (braid + fallback + (base[0] & 7u)) % 37u;
    }

    return acc ^ axis1_wave24_braid_weights[(seed + fallback + braid + 37u) % 74u];
}
