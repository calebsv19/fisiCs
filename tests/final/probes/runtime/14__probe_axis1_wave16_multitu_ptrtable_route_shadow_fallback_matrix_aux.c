extern const unsigned axis1_wave16_shadow_weights[42];
extern const unsigned axis1_wave16_shadow_lane_masks[12];
extern const int axis1_wave16_shadow_signed_offsets[18];
extern const unsigned axis1_wave16_shadow_unsigned_offsets[18];
extern const unsigned* axis1_wave16_shadow_windows[21];
extern const unsigned** axis1_wave16_shadow_routes[11];
extern const unsigned*** axis1_wave16_shadow_plans[3];

unsigned axis1_wave16_ptrtable_route_shadow_fallback_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave16_shadow_weights[(seed + 5u) % 42u];
    unsigned lane = 0u;

    (void)axis1_wave16_shadow_windows;
    (void)axis1_wave16_shadow_routes;

    for (; lane < 12u; ++lane) {
        const unsigned*** plan = axis1_wave16_shadow_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + (seed & 3u)) % 3u];
        const unsigned* base = route[lane & 1u];
        const unsigned* shadow = route[((lane + 1u) ^ (seed & 1u)) & 1u];
        unsigned mask = axis1_wave16_shadow_lane_masks[lane];
        int soff = axis1_wave16_shadow_signed_offsets[(lane + seed + mask) % 18u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave16_shadow_unsigned_offsets[(lane * 2u + mask) % 18u];
        unsigned pick_shadow = ((mask ^ seed ^ uoff) & 1u);
        const unsigned* chosen = pick_shadow ? shadow : base;
        const unsigned* fallback = pick_shadow ? base : shadow;

        acc = ((acc ^ chosen[0]) + fallback[1] + skew) * 31u;
        acc ^= axis1_wave16_shadow_weights[(lane + uoff + chosen[1]) % 42u];
        acc += axis1_wave16_shadow_weights[(lane * 3u + fallback[0] + 7u) % 42u];
    }

    return acc ^ axis1_wave16_shadow_weights[(seed + 23u) % 42u];
}
