extern const unsigned axis1_wave27_arc_weights[86];
extern const unsigned axis1_wave27_arc_lane_masks[22];
extern const int axis1_wave27_arc_signed_offsets[40];
extern const unsigned axis1_wave27_arc_unsigned_offsets[40];
extern const unsigned* axis1_wave27_arc_windows[43];
extern const unsigned** axis1_wave27_arc_routes[16];
extern const unsigned*** axis1_wave27_arc_plans[3];

unsigned axis1_wave27_ptrtable_alias_window_reseed_arc_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave27_arc_weights[(seed + 27u) % 86u];
    unsigned reseed = (seed % 23u) + 2u;
    unsigned arc = (seed % 19u) + 3u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave27_arc_plans[(lane + (seed & 1u) + (arc & 1u)) % 3u];
        const unsigned** route = plan[(lane + reseed + arc) % 5u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + reseed + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave27_arc_lane_masks[(lane + arc) % 22u];
        int soff = axis1_wave27_arc_signed_offsets[(lane + reseed + mask) % 40u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave27_arc_unsigned_offsets[(lane * 2u + reseed + arc + mask) % 40u];

        acc = ((acc ^ base[0]) + alias[1] + skew + reseed) * 113u;
        acc ^= axis1_wave27_arc_weights[(lane + uoff + base[1]) % 86u];
        acc += axis1_wave27_arc_weights[(lane * 3u + alias[0] + 21u) % 86u];
        reseed = (reseed + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 37u;
        arc = (arc + reseed + (base[0] & 7u)) % 43u;
    }

    return acc ^ axis1_wave27_arc_weights[(seed + reseed + arc + 43u) % 86u];
}
