extern const unsigned axis1_wave28_fallback_arc_weights[88];
extern const int axis1_wave28_fallback_arc_signed_offsets[44];
extern const unsigned axis1_wave28_fallback_arc_unsigned_offsets[44];
extern const unsigned* axis1_wave28_fallback_arc_windows[44];
extern const unsigned** axis1_wave28_fallback_arc_routes[16];
extern const unsigned*** axis1_wave28_fallback_arc_plans[4];

unsigned axis1_wave28_ptrtable_checkpoint_route_fallback_arc_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave28_fallback_arc_weights[(seed + 19u) % 88u];
    unsigned checkpoint = (seed % 29u) + 2u;
    unsigned fallback = (seed % 21u) + 3u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave28_fallback_arc_plans[(lane + checkpoint + (fallback & 1u)) % 4u];
        const unsigned** route = plan[(lane + fallback) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alt = route[((lane + checkpoint + fallback) >> 1u) & 1u];
        int soff = axis1_wave28_fallback_arc_signed_offsets[(lane + checkpoint + fallback) % 44u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave28_fallback_arc_unsigned_offsets[(lane * 2u + checkpoint + fallback) % 44u];

        acc = ((acc + base[0] + alt[1] + fallback) ^ skew) * 127u;
        acc ^= axis1_wave28_fallback_arc_weights[(lane + uoff + alt[0]) % 88u];
        fallback = (fallback + (base[1] & 3u) + (alt[0] & 1u) + (lane & 3u)) % 43u;
        checkpoint = (checkpoint + fallback + (base[0] & 7u)) % 53u;
    }

    return acc ^ axis1_wave28_fallback_arc_weights[(seed + checkpoint + fallback + 47u) % 88u];
}
