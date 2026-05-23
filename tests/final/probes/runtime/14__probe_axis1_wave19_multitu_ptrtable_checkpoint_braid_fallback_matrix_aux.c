extern const unsigned axis1_wave19_braid_weights[52];
extern const int axis1_wave19_braid_signed_offsets[26];
extern const unsigned axis1_wave19_braid_unsigned_offsets[26];
extern const unsigned* axis1_wave19_braid_windows[26];
extern const unsigned** axis1_wave19_braid_routes[13];
extern const unsigned*** axis1_wave19_braid_plans[4];

unsigned axis1_wave19_ptrtable_checkpoint_braid_fallback_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave19_braid_weights[(seed + 9u) % 52u];
    unsigned checkpoint = (seed % 13u) + 1u;
    unsigned fallback = (seed % 5u) + 2u;
    unsigned lane = 0u;

    (void)axis1_wave19_braid_windows;
    (void)axis1_wave19_braid_routes;

    for (; lane < 13u; ++lane) {
        const unsigned*** plan = axis1_wave19_braid_plans[(lane + checkpoint) % 4u];
        const unsigned** route = plan[(lane + fallback) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* braid = route[((lane + checkpoint + 1u) >> 1u) & 1u];
        int soff = axis1_wave19_braid_signed_offsets[(lane + checkpoint + fallback) % 26u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave19_braid_unsigned_offsets[(lane * 2u + checkpoint + fallback) % 26u];

        acc = (acc * 41u) ^ (base[0] + braid[1] + skew + checkpoint);
        acc += axis1_wave19_braid_weights[(lane + uoff + braid[0]) % 52u];
        fallback = (fallback + (base[1] & 3u) + (braid[0] & 1u)) % 19u;
        checkpoint = (checkpoint + fallback + (lane & 1u)) % 17u;
    }

    return acc ^ axis1_wave19_braid_weights[(seed + checkpoint + fallback + 19u) % 52u];
}
