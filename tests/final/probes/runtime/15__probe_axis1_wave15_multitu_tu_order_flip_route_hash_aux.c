extern const unsigned axis1_wave15_route_weights[28];
extern const int axis1_wave15_route_signed_offsets[14];
extern const unsigned axis1_wave15_route_unsigned_offsets[14];
extern const unsigned* axis1_wave15_route_windows[14];
extern const unsigned** axis1_wave15_route_routes[10];
extern const unsigned*** axis1_wave15_route_plans[3];
extern unsigned (*axis1_wave15_route_mix)(unsigned, unsigned);

unsigned axis1_wave15_ptrtable_tu_order_flip_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave15_route_windows;
    (void)axis1_wave15_route_routes;

    for (; lane < 9u; ++lane) {
        const unsigned*** plan = axis1_wave15_route_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + (seed & 3u)) % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        int soff = axis1_wave15_route_signed_offsets[(lane + seed) % 14u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave15_route_unsigned_offsets[(lane * 2u + 1u) % 14u];

        acc = axis1_wave15_route_mix(acc + left[0] + skew, right[1] + uoff);
        acc = axis1_wave15_route_mix(acc + axis1_wave15_route_weights[lane + 9u], left[1]);
    }

    return acc ^ axis1_wave15_route_weights[27];
}
