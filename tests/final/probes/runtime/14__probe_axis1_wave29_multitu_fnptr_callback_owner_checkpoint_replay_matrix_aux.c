typedef unsigned (*axis1_wave29_step_fn)(unsigned, unsigned, unsigned);

extern const unsigned axis1_wave29_bias[9];
extern const unsigned axis1_wave29_route_weights[16];
extern const unsigned axis1_wave29_shadow_salts[9];
extern unsigned axis1_wave29_epoch_state;
extern unsigned axis1_wave29_owner_state;
extern axis1_wave29_step_fn axis1_wave29_callback_pick(unsigned lane, unsigned replay);

unsigned axis1_wave29_fnptr_callback_owner_checkpoint_replay_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave29_route_weights[(seed + axis1_wave29_owner_state) % 16u];
    unsigned checkpoint = (seed + axis1_wave29_epoch_state) % 13u;
    unsigned replay = seed & 3u;
    unsigned lane = 0u;

    for (; lane < 9u; ++lane) {
        axis1_wave29_step_fn step = axis1_wave29_callback_pick(lane, replay);
        acc = step(acc + axis1_wave29_bias[lane],
                   axis1_wave29_route_weights[(lane + checkpoint + replay) % 16u],
                   checkpoint + axis1_wave29_shadow_salts[lane]);
        if ((((acc >> ((lane & 3u) + 1u)) ^ checkpoint) & 1u) != 0u) {
            axis1_wave29_owner_state =
                (axis1_wave29_owner_state +
                 axis1_wave29_shadow_salts[(lane + replay) % 9u] +
                 (acc & 3u)) &
                7u;
            checkpoint = (checkpoint + axis1_wave29_owner_state + lane + 1u) % 17u;
        } else {
            axis1_wave29_epoch_state =
                (axis1_wave29_epoch_state +
                 axis1_wave29_bias[(lane + checkpoint) % 9u] +
                 (acc & 7u)) %
                19u;
            replay = (replay + ((acc >> 3u) & 3u) + lane) % 5u;
        }
        acc ^= axis1_wave29_route_weights[(checkpoint + axis1_wave29_epoch_state + lane) % 16u];
    }

    return acc ^ axis1_wave29_route_weights[(checkpoint + replay + axis1_wave29_owner_state) % 16u];
}
