extern const unsigned axis1_wave26_lattice_weights[80];
extern const int axis1_wave26_lattice_signed_offsets[40];
extern const unsigned axis1_wave26_lattice_unsigned_offsets[40];
extern const unsigned* axis1_wave26_lattice_windows[40];
extern const unsigned** axis1_wave26_lattice_routes[16];
extern const unsigned*** axis1_wave26_lattice_plans[4];

unsigned axis1_wave26_ptrtable_checkpoint_route_reseed_lattice_matrix(unsigned seed) {
    unsigned acc = seed ^ axis1_wave26_lattice_weights[(seed + 15u) % 80u];
    unsigned checkpoint = (seed % 29u) + 1u;
    unsigned reseed = (seed % 23u) + 2u;
    unsigned lane = 0u;

    for (; lane < 16u; ++lane) {
        const unsigned*** plan = axis1_wave26_lattice_plans[(lane + checkpoint + (reseed & 1u)) % 4u];
        const unsigned** route = plan[(lane + reseed) % 4u];
        const unsigned* base = route[lane & 1u];
        const unsigned* lattice = route[((lane + reseed + checkpoint) >> 1u) & 1u];
        int soff = axis1_wave26_lattice_signed_offsets[(lane + checkpoint + reseed) % 40u];
        unsigned skew = (unsigned)(soff < 0 ? -soff : soff);
        unsigned uoff = axis1_wave26_lattice_unsigned_offsets[(lane * 2u + checkpoint + reseed) % 40u];

        acc = ((acc + base[0] + lattice[1] + reseed) ^ skew) * 103u;
        acc ^= axis1_wave26_lattice_weights[(lane + uoff + lattice[0]) % 80u];
        reseed = (reseed + (base[1] & 3u) + (lattice[0] & 1u) + (lane & 3u)) % 39u;
        checkpoint = (checkpoint + reseed + (base[0] & 7u)) % 47u;
    }

    return acc ^ axis1_wave26_lattice_weights[(seed + checkpoint + reseed + 35u) % 80u];
}
