extern const unsigned axis1_wave17_alias_weights[44];
extern const int axis1_wave17_alias_signed_offsets[22];
extern const unsigned axis1_wave17_alias_unsigned_offsets[22];
extern const unsigned* axis1_wave17_alias_windows[22];
extern const unsigned** axis1_wave17_alias_routes[11];
extern const unsigned*** axis1_wave17_alias_plans[4];

unsigned axis1_wave17_ptrtable_tu_order_checkpoint_alias_bridge(unsigned seed) {
    unsigned acc = seed ^ axis1_wave17_alias_weights[(seed + 9u) % 44u];
    unsigned checkpoint = (seed % 9u) + 2u;
    unsigned lane = 0u;

    (void)axis1_wave17_alias_windows;
    (void)axis1_wave17_alias_routes;

    for (; lane < 12u; ++lane) {
        const unsigned*** plan = axis1_wave17_alias_plans[(lane + checkpoint) % 4u];
        const unsigned** route = plan[(lane + (seed & 3u)) % 3u];
        const unsigned* left = route[(lane + checkpoint) & 1u];
        const unsigned* right = route[((lane >> 1u) + checkpoint + 1u) & 1u];
        int soff = axis1_wave17_alias_signed_offsets[(lane + seed + checkpoint) % 22u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave17_alias_unsigned_offsets[(lane * 2u + checkpoint + 1u) % 22u];
        unsigned alias_pick = ((checkpoint ^ left[0] ^ uoff) & 1u);
        const unsigned* chosen = alias_pick ? right : left;
        const unsigned* alias = alias_pick ? left : right;

        acc = (acc * 33u) ^ (chosen[0] + alias[1] + skew + checkpoint);
        acc += axis1_wave17_alias_weights[(lane + uoff + alias[0]) % 44u];
        checkpoint = (checkpoint + (chosen[1] & 3u) + (alias[0] & 1u)) % 13u;
    }

    return acc ^ axis1_wave17_alias_weights[(seed + checkpoint + 19u) % 44u];
}
