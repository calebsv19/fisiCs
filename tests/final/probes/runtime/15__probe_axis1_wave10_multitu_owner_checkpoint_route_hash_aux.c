extern const unsigned axis1_wave10_route_weights[16];
extern const unsigned* axis1_wave10_route_windows[8];
extern const unsigned** axis1_wave10_route_routes[6];
extern const unsigned*** axis1_wave10_route_plans[3];
extern unsigned (*axis1_wave10_route_mix)(unsigned, unsigned);

unsigned axis1_wave10_owner_checkpoint_route_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave10_route_windows;
    (void)axis1_wave10_route_routes;

    for (; lane < 6u; ++lane) {
        const unsigned*** plan = axis1_wave10_route_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[lane % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        acc = axis1_wave10_route_mix(acc + left[0], right[1]);
        acc = axis1_wave10_route_mix(acc + axis1_wave10_route_weights[lane + 5u], left[1]);
    }

    return acc ^ axis1_wave10_route_weights[15];
}
