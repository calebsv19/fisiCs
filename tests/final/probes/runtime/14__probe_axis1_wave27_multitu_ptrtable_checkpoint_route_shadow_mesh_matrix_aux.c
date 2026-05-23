extern const unsigned axis1_wave27_shadow_weights[84];
extern const int axis1_wave27_shadow_signed_offsets[42];
extern const unsigned axis1_wave27_shadow_unsigned_offsets[42];
extern const unsigned* axis1_wave27_shadow_windows[42];
extern const unsigned** axis1_wave27_shadow_routes[16];
extern const unsigned*** axis1_wave27_shadow_plans[4];

unsigned axis1_wave27_ptrtable_checkpoint_route_shadow_mesh_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave27_shadow_weights[(seed + 17u) % 84u];
    unsigned checkpoint = (seed % 31u) + 1u;
    unsigned shadow = (seed % 25u) + 2u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave27_shadow_plans[(lane + checkpoint + (shadow & 1u)) % 4u];
        const unsigned** route = plan[(lane + shadow) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* mesh = route[((lane + shadow + checkpoint) >> 1u) & 1u];
        int soff = axis1_wave27_shadow_signed_offsets[(lane + checkpoint + shadow) % 42u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave27_shadow_unsigned_offsets[(lane * 2u + checkpoint + shadow) % 42u];

        acc = ((acc + base[0] + mesh[1] + shadow) ^ skew) * 109u;
        acc ^= axis1_wave27_shadow_weights[(lane + uoff + mesh[0]) % 84u];
        shadow = (shadow + (base[1] & 3u) + (mesh[0] & 1u) + (lane & 3u)) % 41u;
        checkpoint = (checkpoint + shadow + (base[0] & 7u)) % 49u;
    }

    return acc ^ axis1_wave27_shadow_weights[(seed + checkpoint + shadow + 37u) % 84u];
}
