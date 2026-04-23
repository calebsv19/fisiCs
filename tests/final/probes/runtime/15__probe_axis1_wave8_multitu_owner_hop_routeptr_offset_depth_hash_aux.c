extern const unsigned axis1_wave8_hop_weights[12];
extern const unsigned* axis1_wave8_hop_windows[6];
extern const unsigned** axis1_wave8_hop_routes[5];
extern const unsigned*** axis1_wave8_owner_plans[3];
extern unsigned (*axis1_wave8_hop_mix)(unsigned, unsigned);

unsigned axis1_wave8_owner_hop_route_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave8_hop_windows;
    (void)axis1_wave8_hop_routes;

    for (; lane < 5u; ++lane) {
        const unsigned*** plan = axis1_wave8_owner_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[lane % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        acc = axis1_wave8_hop_mix(acc + left[0], right[1]);
        acc = axis1_wave8_hop_mix(acc + axis1_wave8_hop_weights[lane + 2u], left[1]);
    }

    return acc ^ axis1_wave8_hop_weights[11];
}
