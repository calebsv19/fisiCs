extern const unsigned axis1_wave21_collapse_weights[60];
extern const int axis1_wave21_collapse_signed_offsets[30];
extern const unsigned axis1_wave21_collapse_unsigned_offsets[30];
extern const unsigned* axis1_wave21_collapse_windows[30];
extern const unsigned** axis1_wave21_collapse_routes[15];
extern const unsigned*** axis1_wave21_collapse_plans[4];

unsigned axis1_wave21_ptrtable_checkpoint_alias_window_collapse_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave21_collapse_weights[(seed + 3u) % 60u];
    unsigned checkpoint = (seed % 17u) + 1u;
    unsigned collapse = (seed % 11u) + 2u;
    unsigned lane = 0u;

    for (; lane < 15u; ++lane) {
        const unsigned*** plan = axis1_wave21_collapse_plans[(lane + checkpoint) % 4u];
        const unsigned** route = plan[(lane + collapse) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + collapse + 1u) >> 1u) & 1u];
        int soff = axis1_wave21_collapse_signed_offsets[(lane + checkpoint + collapse) % 30u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave21_collapse_unsigned_offsets[(lane * 2u + checkpoint + collapse) % 30u];

        acc = (acc * 59u) ^ (base[0] + alias[1] + skew + checkpoint);
        acc += axis1_wave21_collapse_weights[(lane + uoff + alias[0]) % 60u];
        collapse = (collapse + (base[1] & 3u) + (alias[0] & 1u) + (lane & 1u)) % 23u;
        checkpoint = (checkpoint + collapse + (lane & 3u)) % 27u;
    }

    return acc ^ axis1_wave21_collapse_weights[(seed + checkpoint + collapse + 23u) % 60u];
}
