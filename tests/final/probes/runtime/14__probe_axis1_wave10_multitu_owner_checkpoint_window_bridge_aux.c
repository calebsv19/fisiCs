extern const unsigned axis1_wave10_owner_weights[12];
extern const unsigned* axis1_wave10_owner_windows[6];
extern const unsigned** axis1_wave10_owner_routes[5];
extern unsigned (*axis1_wave10_owner_mix)(unsigned, unsigned);

unsigned axis1_wave10_owner_checkpoint_window_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave10_owner_windows;

    for (; lane < 5u; ++lane) {
        const unsigned** route = axis1_wave10_owner_routes[(lane + (seed & 1u)) % 5u];
        const unsigned* left = route[0];
        const unsigned* right = route[1];
        acc = axis1_wave10_owner_mix(acc + left[0], right[1]);
        acc = axis1_wave10_owner_mix(acc + axis1_wave10_owner_weights[lane + 3u], left[1]);
    }

    return acc ^ axis1_wave10_owner_weights[11];
}
