extern const unsigned axis1_wave15_replay_weights[38];
extern const int axis1_wave15_replay_signed_offsets[18];
extern const unsigned axis1_wave15_replay_unsigned_offsets[18];
extern const unsigned* axis1_wave15_replay_windows[19];
extern const unsigned** axis1_wave15_replay_routes[10];
extern const unsigned*** axis1_wave15_replay_plans[3];

unsigned axis1_wave15_ptrtable_checkpoint_replay_permute(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave15_replay_windows;

    for (; lane < 10u; ++lane) {
        const unsigned*** plan = axis1_wave15_replay_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + (seed & 3u)) % 3u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        int soff = axis1_wave15_replay_signed_offsets[(lane + seed) % 18u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave15_replay_unsigned_offsets[(lane * 2u + 3u) % 18u];

        acc = ((acc + left[0] + right[1] + skew) ^ uoff) * 23u;
        acc += axis1_wave15_replay_weights[(lane + uoff) % 38u];
        acc ^= axis1_wave15_replay_weights[(lane * 3u + skew + 13u) % 38u];
    }

    return acc + axis1_wave15_replay_weights[(seed + 19u) % 38u];
}
