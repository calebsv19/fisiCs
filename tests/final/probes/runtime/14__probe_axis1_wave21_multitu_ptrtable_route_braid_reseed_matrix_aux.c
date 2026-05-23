extern const unsigned axis1_wave21_braid_weights[62];
extern const unsigned axis1_wave21_braid_lane_masks[15];
extern const int axis1_wave21_braid_signed_offsets[28];
extern const unsigned axis1_wave21_braid_unsigned_offsets[28];
extern const unsigned* axis1_wave21_braid_windows[31];
extern const unsigned** axis1_wave21_braid_routes[15];
extern const unsigned*** axis1_wave21_braid_plans[3];

unsigned axis1_wave21_ptrtable_route_braid_reseed_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave21_braid_weights[(seed + 13u) % 62u];
    unsigned braid = (seed % 11u) + 2u;
    unsigned lane = 0u;

    for (; lane < 15u; ++lane) {
        const unsigned*** plan = axis1_wave21_braid_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + braid) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* mirror = route[((lane + braid + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave21_braid_lane_masks[lane];
        int soff = axis1_wave21_braid_signed_offsets[(lane + braid + mask) % 28u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave21_braid_unsigned_offsets[(lane * 2u + braid + mask) % 28u];

        acc = ((acc ^ base[0]) + mirror[1] + skew + braid) * 61u;
        acc ^= axis1_wave21_braid_weights[(lane + uoff + base[1]) % 62u];
        acc += axis1_wave21_braid_weights[(lane * 3u + mirror[0] + 9u) % 62u];
        braid = (braid + (base[1] & 3u) + (mirror[0] & 1u) + (mask & 1u)) % 23u;
    }

    return acc ^ axis1_wave21_braid_weights[(seed + braid + 31u) % 62u];
}
