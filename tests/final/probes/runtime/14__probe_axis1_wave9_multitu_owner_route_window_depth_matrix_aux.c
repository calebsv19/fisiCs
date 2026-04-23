extern const unsigned axis1_wave9_owner_weights[10];
extern const unsigned* axis1_wave9_owner_windows[5];
extern const unsigned* const* axis1_wave9_owner_routes[4];
extern unsigned (*axis1_wave9_owner_mix)(unsigned, unsigned);

unsigned axis1_wave9_owner_route_window_depth(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave9_owner_windows;

    for (; lane < 4u; ++lane) {
        const unsigned* const* route = axis1_wave9_owner_routes[(lane + (seed & 1u)) & 3u];
        const unsigned* left = route[0];
        const unsigned* right = route[1];
        acc = axis1_wave9_owner_mix(acc + left[0], right[1]);
        acc = axis1_wave9_owner_mix(acc + axis1_wave9_owner_weights[lane + 2u], left[1]);
    }

    return acc ^ axis1_wave9_owner_weights[9];
}
