extern const unsigned axis1_wave23_collapse_weights[68];
extern const int axis1_wave23_collapse_signed_offsets[34];
extern const unsigned axis1_wave23_collapse_unsigned_offsets[34];
extern const unsigned* axis1_wave23_collapse_windows[34];
extern const unsigned** axis1_wave23_collapse_routes[16];
extern const unsigned*** axis1_wave23_collapse_plans[4];

unsigned axis1_wave23_ptrtable_checkpoint_route_braid_collapse_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave23_collapse_weights[(seed + 9u) % 68u];
    unsigned checkpoint = (seed % 23u) + 1u;
    unsigned collapse = (seed % 17u) + 3u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave23_collapse_plans[(lane + checkpoint + (collapse & 1u)) % 4u];
        const unsigned** route = plan[(lane + collapse) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* braid = route[((lane + collapse + checkpoint) >> 1u) & 1u];
        int soff = axis1_wave23_collapse_signed_offsets[(lane + checkpoint + collapse) % 34u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave23_collapse_unsigned_offsets[(lane * 2u + checkpoint + collapse) % 34u];

        acc = ((acc + base[0] + braid[1] + checkpoint) ^ skew) * 73u;
        acc ^= axis1_wave23_collapse_weights[(lane + uoff + braid[0]) % 68u];
        collapse = (collapse + (base[1] & 3u) + (braid[0] & 1u) + (lane & 3u)) % 31u;
        checkpoint = (checkpoint + collapse + (base[0] & 7u)) % 37u;
    }

    return acc ^ axis1_wave23_collapse_weights[(seed + checkpoint + collapse + 29u) % 68u];
}
