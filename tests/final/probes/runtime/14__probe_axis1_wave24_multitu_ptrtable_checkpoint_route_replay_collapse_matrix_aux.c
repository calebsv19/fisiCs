extern const unsigned axis1_wave24_replay_weights[72];
extern const int axis1_wave24_replay_signed_offsets[36];
extern const unsigned axis1_wave24_replay_unsigned_offsets[36];
extern const unsigned* axis1_wave24_replay_windows[36];
extern const unsigned** axis1_wave24_replay_routes[16];
extern const unsigned*** axis1_wave24_replay_plans[4];

unsigned axis1_wave24_ptrtable_checkpoint_route_replay_collapse_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave24_replay_weights[(seed + 11u) % 72u];
    unsigned checkpoint = (seed % 25u) + 1u;
    unsigned replay = (seed % 19u) + 2u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave24_replay_plans[(lane + checkpoint + (replay & 1u)) % 4u];
        const unsigned** route = plan[(lane + replay) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* collapse = route[((lane + replay + checkpoint) >> 1u) & 1u];
        int soff = axis1_wave24_replay_signed_offsets[(lane + checkpoint + replay) % 36u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave24_replay_unsigned_offsets[(lane * 2u + checkpoint + replay) % 36u];

        acc = ((acc + base[0] + collapse[1] + replay) ^ skew) * 83u;
        acc ^= axis1_wave24_replay_weights[(lane + uoff + collapse[0]) % 72u];
        replay = (replay + (base[1] & 3u) + (collapse[0] & 1u) + (lane & 3u)) % 35u;
        checkpoint = (checkpoint + replay + (base[0] & 7u)) % 41u;
    }

    return acc ^ axis1_wave24_replay_weights[(seed + checkpoint + replay + 31u) % 72u];
}
