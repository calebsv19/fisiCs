extern const unsigned axis1_wave13_route_weights[24];
extern const unsigned* axis1_wave13_route_windows[12];
extern const unsigned** axis1_wave13_route_routes[8];
extern const unsigned*** axis1_wave13_route_checkpoints[6];
extern const unsigned**** axis1_wave13_route_plans[3];
extern unsigned (*axis1_wave13_route_mix)(unsigned, unsigned);

unsigned axis1_wave13_ptrtable_four_level_route_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave13_route_windows;
    (void)axis1_wave13_route_routes;
    (void)axis1_wave13_route_checkpoints;

    for (; lane < 8u; ++lane) {
        const unsigned**** plan = axis1_wave13_route_plans[(lane + (seed & 1u)) % 3u];
        const unsigned*** checkpoint = plan[lane & 1u];
        const unsigned** route = checkpoint[(lane + 1u) & 1u];
        const unsigned* left = route[lane & 1u];
        const unsigned* right = route[(lane + 1u) & 1u];

        acc = axis1_wave13_route_mix(acc + left[0], right[1]);
        acc = axis1_wave13_route_mix(acc + axis1_wave13_route_weights[lane + 7u], left[1]);
    }

    return acc ^ axis1_wave13_route_weights[23];
}
