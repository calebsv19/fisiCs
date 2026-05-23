typedef unsigned (*axis1_wave30_owner_fn)(unsigned, const unsigned*, unsigned);

extern const unsigned axis1_wave30_owner_weights[18];
extern const unsigned* axis1_wave30_owner_windows[7];
extern const unsigned** axis1_wave30_owner_routes[6];
extern const unsigned axis1_wave30_handoff_perm[8];
extern unsigned axis1_wave30_route_state;
extern unsigned axis1_wave30_epoch_state;
extern axis1_wave30_owner_fn axis1_wave30_owner_pick(unsigned lane, unsigned checkpoint);

unsigned axis1_wave30_owner_window_callback_handoff_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave30_owner_weights[(seed + axis1_wave30_route_state) % 18u];
    unsigned checkpoint = (seed ^ axis1_wave30_epoch_state) % 11u;
    unsigned handoff = seed & 1u;
    unsigned lane = 0u;

    (void)axis1_wave30_owner_windows;

    for (; lane < 8u; ++lane) {
        const unsigned** route =
            axis1_wave30_owner_routes[(lane + axis1_wave30_route_state + handoff) % 6u];
        const unsigned* left = route[0];
        const unsigned* right = route[1];
        axis1_wave30_owner_fn step = axis1_wave30_owner_pick(lane, checkpoint);
        acc = step(acc + left[0], right, checkpoint + axis1_wave30_handoff_perm[lane]);
        if (((acc >> ((lane & 2u) + 1u)) & 1u) != 0u) {
            axis1_wave30_route_state =
                (axis1_wave30_route_state + right[0] + (acc & 3u) + lane) % 9u;
            checkpoint = (checkpoint + left[1] + axis1_wave30_route_state) % 17u;
        } else {
            axis1_wave30_epoch_state =
                (axis1_wave30_epoch_state + right[1] + (acc & 7u) + handoff) % 19u;
            handoff = (handoff + axis1_wave30_handoff_perm[(lane + checkpoint) % 8u]) & 3u;
        }
        acc ^= axis1_wave30_owner_weights[(checkpoint + axis1_wave30_epoch_state + lane) % 18u];
    }

    return acc ^ axis1_wave30_owner_weights[(checkpoint + handoff + axis1_wave30_route_state) % 18u];
}
