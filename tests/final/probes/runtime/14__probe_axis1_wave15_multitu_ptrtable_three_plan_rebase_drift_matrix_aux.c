extern const unsigned axis1_wave15_weights[36];
extern const int axis1_wave15_signed_offsets[16];
extern const unsigned axis1_wave15_unsigned_offsets[16];
extern const unsigned* axis1_wave15_windows[18];
extern const unsigned** axis1_wave15_routes[10];
extern const unsigned*** axis1_wave15_plans[3];

unsigned axis1_wave15_ptrtable_three_plan_rebase_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave15_windows;
    (void)axis1_wave15_routes;

    for (; lane < 10u; ++lane) {
        const unsigned*** plan = axis1_wave15_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + (seed & 3u)) % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        int soff = axis1_wave15_signed_offsets[(lane + seed) % 16u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave15_unsigned_offsets[(lane * 3u + 2u) % 16u];

        acc = ((acc + left[0] + right[1] + skew) ^ uoff) * 19u;
        acc += axis1_wave15_weights[(lane + uoff) % 36u];
        acc ^= axis1_wave15_weights[(lane * 2u + skew + 11u) % 36u];
    }

    return acc ^ axis1_wave15_weights[(seed + 17u) % 36u];
}
