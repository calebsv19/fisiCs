typedef unsigned (*axis1_wave31_route_fn)(unsigned, unsigned, unsigned);

extern const unsigned axis1_wave31_owner_perm[10];
extern const unsigned axis1_wave31_owner_bias[10];
extern const unsigned axis1_wave31_route_weights[20];
extern unsigned axis1_wave31_shadow_state;
extern unsigned axis1_wave31_checkpoint_state;
extern axis1_wave31_route_fn axis1_wave31_pick(unsigned lane, unsigned epoch);

unsigned axis1_wave31_fnptr_owner_route_shadow_checkpoint_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave31_route_weights[(seed + axis1_wave31_shadow_state) % 20u];
    unsigned epoch = (seed + axis1_wave31_checkpoint_state) % 15u;
    unsigned lane = 0u;

    for (; lane < 10u; ++lane) {
        axis1_wave31_route_fn step = axis1_wave31_pick(lane, epoch);
        unsigned weight = axis1_wave31_route_weights[(lane + epoch + axis1_wave31_shadow_state) % 20u];
        acc = step(acc + axis1_wave31_owner_bias[lane], weight, epoch + axis1_wave31_owner_perm[lane]);
        if (((acc ^ weight) & 1u) != 0u) {
            axis1_wave31_shadow_state =
                (axis1_wave31_shadow_state + axis1_wave31_owner_perm[(lane + epoch) % 10u] + (acc & 3u)) %
                23u;
        } else {
            axis1_wave31_checkpoint_state =
                (axis1_wave31_checkpoint_state + axis1_wave31_owner_bias[(lane + axis1_wave31_shadow_state) % 10u] +
                 (acc & 7u)) %
                17u;
        }
        epoch = (epoch + axis1_wave31_checkpoint_state + lane + 1u) % 21u;
        acc ^= axis1_wave31_route_weights[(epoch + axis1_wave31_shadow_state + lane) % 20u];
    }

    return acc ^ axis1_wave31_route_weights[(epoch + axis1_wave31_checkpoint_state) % 20u];
}
