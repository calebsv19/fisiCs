extern const unsigned axis1_wave25_mesh_weights[76];
extern const int axis1_wave25_mesh_signed_offsets[38];
extern const unsigned axis1_wave25_mesh_unsigned_offsets[38];
extern const unsigned* axis1_wave25_mesh_windows[38];
extern const unsigned** axis1_wave25_mesh_routes[16];
extern const unsigned*** axis1_wave25_mesh_plans[4];

unsigned axis1_wave25_ptrtable_checkpoint_route_fallback_mesh_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave25_mesh_weights[(seed + 13u) % 76u];
    unsigned checkpoint = (seed % 27u) + 1u;
    unsigned fallback = (seed % 21u) + 2u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave25_mesh_plans[(lane + checkpoint + (fallback & 1u)) % 4u];
        const unsigned** route = plan[(lane + fallback) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* mesh = route[((lane + fallback + checkpoint) >> 1u) & 1u];
        int soff = axis1_wave25_mesh_signed_offsets[(lane + checkpoint + fallback) % 38u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave25_mesh_unsigned_offsets[(lane * 2u + checkpoint + fallback) % 38u];

        acc = ((acc + base[0] + mesh[1] + fallback) ^ skew) * 97u;
        acc ^= axis1_wave25_mesh_weights[(lane + uoff + mesh[0]) % 76u];
        fallback = (fallback + (base[1] & 3u) + (mesh[0] & 1u) + (lane & 3u)) % 37u;
        checkpoint = (checkpoint + fallback + (base[0] & 7u)) % 43u;
    }

    return acc ^ axis1_wave25_mesh_weights[(seed + checkpoint + fallback + 33u) % 76u];
}
