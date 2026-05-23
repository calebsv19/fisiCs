extern const unsigned axis1_wave22_braid_weights[66];
extern const unsigned axis1_wave22_braid_lane_masks[16];
extern const int axis1_wave22_braid_signed_offsets[30];
extern const unsigned axis1_wave22_braid_unsigned_offsets[30];
extern const unsigned* axis1_wave22_braid_windows[33];
extern const unsigned** axis1_wave22_braid_routes[16];
extern const unsigned*** axis1_wave22_braid_plans[3];

unsigned axis1_wave22_ptrtable_alias_replay_braid_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave22_braid_weights[(seed + 17u) % 66u];
    unsigned replay = (seed % 13u) + 2u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave22_braid_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + replay) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* braid = route[((lane + replay + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave22_braid_lane_masks[lane];
        int soff = axis1_wave22_braid_signed_offsets[(lane + replay + mask) % 30u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave22_braid_unsigned_offsets[(lane * 2u + replay + mask) % 30u];

        acc = ((acc ^ base[0]) + braid[1] + skew + replay) * 71u;
        acc ^= axis1_wave22_braid_weights[(lane + uoff + base[1]) % 66u];
        acc += axis1_wave22_braid_weights[(lane * 3u + braid[0] + 11u) % 66u];
        replay = (replay + (base[1] & 3u) + (braid[0] & 1u) + (mask & 1u)) % 27u;
    }

    return acc ^ axis1_wave22_braid_weights[(seed + replay + 33u) % 66u];
}
