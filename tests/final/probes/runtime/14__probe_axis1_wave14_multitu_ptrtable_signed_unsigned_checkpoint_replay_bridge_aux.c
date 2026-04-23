extern const unsigned axis1_wave14_replay_weights[34];
extern const int axis1_wave14_replay_signed_offsets[16];
extern const unsigned axis1_wave14_replay_unsigned_offsets[16];
extern const unsigned* axis1_wave14_replay_windows[17];
extern const unsigned** axis1_wave14_replay_routes[10];

unsigned axis1_wave14_ptrtable_checkpoint_replay_bridge(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave14_replay_windows;

    for (; lane < 9u; ++lane) {
        const unsigned** route = axis1_wave14_replay_routes[(lane + (seed & 3u)) % 10u];
        const unsigned* left = route[(lane + 1u) & 1u];
        const unsigned* right = route[lane & 1u];
        int soff = axis1_wave14_replay_signed_offsets[(lane + seed) % 16u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave14_replay_unsigned_offsets[(lane * 3u + 2u) % 16u];

        acc = ((acc + left[0] + right[1] + skew) ^ uoff) * 17u;
        acc += axis1_wave14_replay_weights[(lane + uoff) % 34u];
        acc ^= axis1_wave14_replay_weights[(lane * 2u + skew + 9u) % 34u];
    }

    return acc + axis1_wave14_replay_weights[(seed + 15u) % 34u];
}
