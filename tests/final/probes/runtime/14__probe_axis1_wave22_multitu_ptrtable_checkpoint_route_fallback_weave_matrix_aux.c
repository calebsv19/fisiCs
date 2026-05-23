extern const unsigned axis1_wave22_weave_weights[64];
extern const int axis1_wave22_weave_signed_offsets[32];
extern const unsigned axis1_wave22_weave_unsigned_offsets[32];
extern const unsigned* axis1_wave22_weave_windows[32];
extern const unsigned** axis1_wave22_weave_routes[16];
extern const unsigned*** axis1_wave22_weave_plans[4];

unsigned axis1_wave22_ptrtable_checkpoint_route_fallback_weave_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave22_weave_weights[(seed + 7u) % 64u];
    unsigned checkpoint = (seed % 19u) + 1u;
    unsigned fallback = (seed % 13u) + 2u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave22_weave_plans[(lane + checkpoint) % 4u];
        const unsigned** route = plan[(lane + fallback) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* weave = route[((lane + fallback + 1u) >> 1u) & 1u];
        int soff = axis1_wave22_weave_signed_offsets[(lane + checkpoint + fallback) % 32u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave22_weave_unsigned_offsets[(lane * 2u + checkpoint + fallback) % 32u];

        acc = (acc * 67u) ^ (base[0] + weave[1] + skew + checkpoint);
        acc += axis1_wave22_weave_weights[(lane + uoff + weave[0]) % 64u];
        fallback = (fallback + (base[1] & 3u) + (weave[0] & 1u) + (lane & 1u)) % 29u;
        checkpoint = (checkpoint + fallback + (lane & 3u)) % 31u;
    }

    return acc ^ axis1_wave22_weave_weights[(seed + checkpoint + fallback + 27u) % 64u];
}
