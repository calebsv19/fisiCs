extern const unsigned axis1_wave11_route_weights[18];
extern const unsigned* axis1_wave11_route_windows[9];
extern const unsigned** axis1_wave11_route_routes[6];
extern const unsigned*** axis1_wave11_route_plans[3];
extern unsigned (*axis1_wave11_route_mix)(unsigned, unsigned);

unsigned axis1_wave11_ptrtable_nested_decay_route_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave11_route_windows;
    (void)axis1_wave11_route_routes;

    for (; lane < 7u; ++lane) {
        const unsigned*** plan = axis1_wave11_route_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[lane % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];

        acc = axis1_wave11_route_mix(acc + left[0], right[1]);
        acc = axis1_wave11_route_mix(acc + axis1_wave11_route_weights[lane + 4u], left[1]);
    }

    return acc ^ axis1_wave11_route_weights[17];
}
