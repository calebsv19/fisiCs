extern const unsigned axis1_wave14_route_weights[26];
extern const int axis1_wave14_route_signed_offsets[12];
extern const unsigned axis1_wave14_route_unsigned_offsets[12];
extern const unsigned* axis1_wave14_route_windows[13];
extern const unsigned** axis1_wave14_route_routes[8];
extern const unsigned*** axis1_wave14_route_checkpoints[6];
extern unsigned (*axis1_wave14_route_mix)(unsigned, unsigned);

unsigned axis1_wave14_ptrtable_relocation_order_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave14_route_windows;

    for (; lane < 8u; ++lane) {
        const unsigned*** checkpoint = axis1_wave14_route_checkpoints[(lane + (seed & 1u)) % 6u];
        const unsigned** route = checkpoint[lane & 1u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        int soff = axis1_wave14_route_signed_offsets[(lane + seed) % 12u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave14_route_unsigned_offsets[(lane * 2u + 1u) % 12u];

        acc = axis1_wave14_route_mix(acc + left[0] + skew, right[1] + uoff);
        acc = axis1_wave14_route_mix(acc + axis1_wave14_route_weights[lane + 8u], left[1]);
    }

    return acc ^ axis1_wave14_route_weights[25];
}
