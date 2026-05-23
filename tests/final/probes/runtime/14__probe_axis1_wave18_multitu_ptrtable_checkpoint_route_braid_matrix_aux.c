extern const unsigned axis1_wave18_braid_weights[48];
extern const int axis1_wave18_braid_signed_offsets[24];
extern const unsigned axis1_wave18_braid_unsigned_offsets[24];
extern const unsigned* axis1_wave18_braid_windows[24];
extern const unsigned** axis1_wave18_braid_routes[12];
extern const unsigned*** axis1_wave18_braid_plans[4];

unsigned axis1_wave18_ptrtable_checkpoint_route_braid_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave18_braid_weights[(seed + 11u) % 48u];
    unsigned checkpoint = (seed % 11u) + 1u;
    unsigned lane = 0u;

    (void)axis1_wave18_braid_windows;
    (void)axis1_wave18_braid_routes;

    for (; lane < 12u; ++lane) {
        const unsigned*** plan = axis1_wave18_braid_plans[(lane + checkpoint) % 4u];
        const unsigned** route = plan[(lane + (seed & 3u)) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* braid = route[((lane + checkpoint + 1u) >> 1u) & 1u];
        int soff = axis1_wave18_braid_signed_offsets[(lane + seed + checkpoint) % 24u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave18_braid_unsigned_offsets[(lane * 2u + checkpoint + 1u) % 24u];

        acc = (acc * 37u) ^ (base[0] + braid[1] + skew + checkpoint);
        acc += axis1_wave18_braid_weights[(lane + uoff + braid[0]) % 48u];
        checkpoint = (checkpoint + (base[1] & 3u) + (braid[0] & 1u)) % 17u;
    }

    return acc ^ axis1_wave18_braid_weights[(seed + checkpoint + 23u) % 48u];
}
