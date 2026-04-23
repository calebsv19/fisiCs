extern const unsigned axis1_wave7_owner_weights[8];
extern const unsigned* axis1_wave7_owner_windows[3];
extern const unsigned* const* axis1_wave7_owner_routes[3];
extern unsigned (*axis1_wave7_owner_mix)(unsigned, unsigned);

unsigned axis1_wave7_owner_depth_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave7_owner_windows;

    for (; lane < 3u; ++lane) {
        const unsigned* const* route = axis1_wave7_owner_routes[(lane + (seed & 1u)) % 3u];
        const unsigned* window = route[0];
        acc = axis1_wave7_owner_mix(acc + axis1_wave7_owner_weights[lane], window[1]);
        acc = axis1_wave7_owner_mix(acc + axis1_wave7_owner_weights[lane + 3u], window[0]);
    }

    return acc ^ axis1_wave7_owner_weights[7];
}
