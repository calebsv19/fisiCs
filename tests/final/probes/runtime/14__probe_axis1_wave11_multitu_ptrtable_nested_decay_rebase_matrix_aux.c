extern const unsigned axis1_wave11_decay_weights[20];
extern const unsigned* axis1_wave11_decay_windows[10];
extern const unsigned** axis1_wave11_decay_routes[6];
extern const unsigned*** axis1_wave11_decay_plans[4];
extern const unsigned axis1_wave11_decay_bias[8];

unsigned axis1_wave11_ptrtable_nested_decay_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave11_decay_routes;

    for (; lane < 8u; ++lane) {
        const unsigned*** plan = axis1_wave11_decay_plans[(lane + (seed & 3u)) & 3u];
        const unsigned** route = plan[lane % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[(lane + 2u) & 1u];
        const unsigned* base = axis1_wave11_decay_windows[(lane + (seed & 1u)) % 10u];

        acc = (acc + left[0] + right[1] + base[0] + axis1_wave11_decay_bias[lane]) * 5u;
        acc ^= axis1_wave11_decay_weights[(lane * 2u + 3u) % 20u];
    }

    return acc ^ axis1_wave11_decay_bias[seed & 7u];
}
