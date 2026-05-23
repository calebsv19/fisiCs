extern const unsigned axis1_wave20_alias_weights[58];
extern const unsigned axis1_wave20_alias_lane_masks[14];
extern const int axis1_wave20_alias_signed_offsets[26];
extern const unsigned axis1_wave20_alias_unsigned_offsets[26];
extern const unsigned* axis1_wave20_alias_windows[29];
extern const unsigned** axis1_wave20_alias_routes[14];
extern const unsigned*** axis1_wave20_alias_plans[3];

unsigned axis1_wave20_ptrtable_alias_window_braid_reseed_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave20_alias_weights[(seed + 11u) % 58u];
    unsigned braid = (seed % 9u) + 2u;
    unsigned lane = 0u;

    for (; lane < 14u; ++lane) {
        const unsigned*** plan = axis1_wave20_alias_plans[(lane + (seed & 1u)) % 3u];
        const unsigned** route = plan[(lane + braid) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + braid + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave20_alias_lane_masks[lane];
        int soff = axis1_wave20_alias_signed_offsets[(lane + braid + mask) % 26u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave20_alias_unsigned_offsets[(lane * 2u + braid + mask) % 26u];

        acc = ((acc ^ base[0]) + alias[1] + skew + braid) * 53u;
        acc ^= axis1_wave20_alias_weights[(lane + uoff + base[1]) % 58u];
        acc += axis1_wave20_alias_weights[(lane * 3u + alias[0] + 7u) % 58u];
        braid = (braid + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 21u;
    }

    return acc ^ axis1_wave20_alias_weights[(seed + braid + 29u) % 58u];
}
