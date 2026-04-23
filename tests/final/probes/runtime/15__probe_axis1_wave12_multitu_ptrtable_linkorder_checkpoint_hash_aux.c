extern const unsigned axis1_wave12_route_weights[20];
extern const int axis1_wave12_route_signed_offsets[10];
extern const unsigned axis1_wave12_route_unsigned_offsets[10];
extern const unsigned* axis1_wave12_route_windows[10];
extern const unsigned** axis1_wave12_route_routes[6];
extern const unsigned*** axis1_wave12_route_plans[3];
extern unsigned (*axis1_wave12_route_mix)(unsigned, unsigned);

unsigned axis1_wave12_ptrtable_linkorder_checkpoint_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave12_route_windows;
    (void)axis1_wave12_route_routes;

    for (; lane < 7u; ++lane) {
        int soff = axis1_wave12_route_signed_offsets[(lane + seed) % 10u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave12_route_unsigned_offsets[(lane * 2u + 1u) % 10u];
        const unsigned*** plan = axis1_wave12_route_plans[(lane + (uoff & 1u)) % 3u];
        const unsigned** route = plan[lane % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];

        acc = axis1_wave12_route_mix(acc + left[0] + skew, right[1] + uoff);
        acc = axis1_wave12_route_mix(acc + axis1_wave12_route_weights[lane + 6u], left[1]);
    }

    return acc ^ axis1_wave12_route_weights[19];
}
