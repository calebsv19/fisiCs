extern const unsigned axis1_wave26_mesh_weights[82];
extern const unsigned axis1_wave26_mesh_lane_masks[22];
extern const int axis1_wave26_mesh_signed_offsets[38];
extern const unsigned axis1_wave26_mesh_unsigned_offsets[38];
extern const unsigned* axis1_wave26_mesh_windows[41];
extern const unsigned** axis1_wave26_mesh_routes[16];
extern const unsigned*** axis1_wave26_mesh_plans[3];

unsigned axis1_wave26_ptrtable_alias_window_fallback_mesh_matrix(unsigned seed) {
    unsigned acc = seed + axis1_wave26_mesh_weights[(seed + 25u) % 82u];
    unsigned fallback = (seed % 21u) + 2u;
    unsigned mesh = (seed % 17u) + 3u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave26_mesh_plans[(lane + (seed & 1u) + (mesh & 1u)) % 3u];
        const unsigned** route = plan[(lane + fallback + mesh) % 5u];
        const unsigned* base = route[lane & 1u];
        const unsigned* alias = route[((lane + fallback + 1u) >> 1u) & 1u];
        unsigned mask = axis1_wave26_mesh_lane_masks[(lane + mesh) % 22u];
        int soff = axis1_wave26_mesh_signed_offsets[(lane + fallback + mask) % 38u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave26_mesh_unsigned_offsets[(lane * 2u + fallback + mesh + mask) % 38u];

        acc = ((acc ^ base[0]) + alias[1] + skew + fallback) * 107u;
        acc ^= axis1_wave26_mesh_weights[(lane + uoff + base[1]) % 82u];
        acc += axis1_wave26_mesh_weights[(lane * 3u + alias[0] + 19u) % 82u];
        fallback = (fallback + (base[1] & 3u) + (alias[0] & 1u) + (mask & 1u)) % 35u;
        mesh = (mesh + fallback + (base[0] & 7u)) % 41u;
    }

    return acc ^ axis1_wave26_mesh_weights[(seed + fallback + mesh + 41u) % 82u];
}
