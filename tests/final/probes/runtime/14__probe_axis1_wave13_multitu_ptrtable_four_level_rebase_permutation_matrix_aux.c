extern const unsigned axis1_wave13_weights[32];
extern const unsigned* axis1_wave13_windows[16];
extern const unsigned** axis1_wave13_routes[8];
extern const unsigned*** axis1_wave13_checkpoints[6];
extern const unsigned**** axis1_wave13_plans[3];

unsigned axis1_wave13_ptrtable_four_level_rebase_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave13_windows;
    (void)axis1_wave13_routes;
    (void)axis1_wave13_checkpoints;

    for (; lane < 9u; ++lane) {
        const unsigned**** plan = axis1_wave13_plans[(lane + (seed & 1u)) % 3u];
        const unsigned*** checkpoint = plan[lane & 1u];
        const unsigned** route = checkpoint[(lane + 1u) & 1u];
        const unsigned* left = route[lane & 1u];
        const unsigned* right = route[(lane + 1u) & 1u];

        acc = ((acc + left[0] + right[1]) ^ axis1_wave13_weights[(lane * 3u + 5u) % 32u]) * 7u;
        acc += axis1_wave13_weights[(lane * 2u + (seed & 7u)) % 32u];
    }

    return acc ^ axis1_wave13_weights[(seed + 11u) % 32u];
}
