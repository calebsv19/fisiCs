extern const unsigned axis1_wave32_owner_weights[22];
extern const unsigned* axis1_wave32_owner_windows[8];
extern const unsigned** axis1_wave32_owner_routes[7];
extern const unsigned axis1_wave32_fallback_perm[9];
extern unsigned axis1_wave32_route_state;
extern unsigned axis1_wave32_replay_state;
extern unsigned axis1_wave32_owner_mix(unsigned value, unsigned weight, unsigned salt);

unsigned axis1_wave32_owner_window_replay_fallback_handoff_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave32_owner_weights[(seed + axis1_wave32_route_state) % 22u];
    unsigned checkpoint = (seed ^ axis1_wave32_replay_state) % 13u;
    unsigned lane = 0u;

    (void)axis1_wave32_owner_windows;

    for (; lane < 9u; ++lane) {
        const unsigned** route =
            axis1_wave32_owner_routes[(lane + axis1_wave32_route_state + axis1_wave32_replay_state) % 7u];
        const unsigned* primary = route[0];
        const unsigned* secondary = route[(checkpoint + lane) & 1u];
        unsigned salt =
            axis1_wave32_fallback_perm[(lane + checkpoint + axis1_wave32_replay_state) % 9u] +
            axis1_wave32_route_state;
        acc = axis1_wave32_owner_mix(acc + primary[0], secondary[1], salt);
        if (((acc >> ((lane & 3u) + 1u)) & 1u) != 0u) {
            axis1_wave32_route_state =
                (axis1_wave32_route_state + secondary[0] + (acc & 3u) + lane) % 11u;
        } else {
            axis1_wave32_replay_state =
                (axis1_wave32_replay_state + primary[1] + (acc & 7u) + checkpoint) % 13u;
        }
        checkpoint =
            (checkpoint + axis1_wave32_fallback_perm[(lane + axis1_wave32_route_state) % 9u] +
             axis1_wave32_replay_state) %
            19u;
        acc ^= axis1_wave32_owner_weights[(checkpoint + lane + axis1_wave32_route_state) % 22u];
    }

    return acc ^ axis1_wave32_owner_weights[(checkpoint + axis1_wave32_replay_state) % 22u];
}
