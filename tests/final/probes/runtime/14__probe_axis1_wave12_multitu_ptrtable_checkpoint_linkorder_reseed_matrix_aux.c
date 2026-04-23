extern const unsigned axis1_wave12_checkpoint_weights[22];
extern const int axis1_wave12_checkpoint_signed_offsets[10];
extern const unsigned axis1_wave12_checkpoint_unsigned_offsets[10];
extern const unsigned* axis1_wave12_checkpoint_windows[11];
extern const unsigned** axis1_wave12_checkpoint_routes[6];
extern const unsigned*** axis1_wave12_checkpoint_plans[4];

unsigned axis1_wave12_ptrtable_checkpoint_linkorder_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave12_checkpoint_routes;

    for (; lane < 8u; ++lane) {
        int soff = axis1_wave12_checkpoint_signed_offsets[(lane + seed) % 10u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave12_checkpoint_unsigned_offsets[(lane * 2u + 1u) % 10u];
        const unsigned*** plan = axis1_wave12_checkpoint_plans[(lane + (uoff & 1u)) & 3u];
        const unsigned** route = plan[lane % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[(lane + 2u) & 1u];
        const unsigned* base = axis1_wave12_checkpoint_windows[(lane + (uoff & 3u)) % 11u];

        acc = (acc + left[0] + right[1] + base[0] + uoff + skew)
            ^ axis1_wave12_checkpoint_weights[(lane + 3u) % 22u];
        acc = (acc * 7u) + axis1_wave12_checkpoint_weights[(lane * 2u + skew) % 22u];
    }

    return acc ^ axis1_wave12_checkpoint_weights[(seed + 5u) % 22u];
}
