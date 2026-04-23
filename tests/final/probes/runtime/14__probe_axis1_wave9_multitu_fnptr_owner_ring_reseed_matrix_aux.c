typedef unsigned (*axis1_wave9_ring_fn)(unsigned, unsigned);

extern axis1_wave9_ring_fn axis1_wave9_ring_table[4];
extern const unsigned axis1_wave9_ring_perm[5];
extern const unsigned axis1_wave9_ring_bias[5];

unsigned axis1_wave9_fnptr_owner_ring_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    for (; lane < 5u; ++lane) {
        unsigned pick = axis1_wave9_ring_perm[(lane + (seed & 1u)) % 5u] & 3u;
        axis1_wave9_ring_fn step = axis1_wave9_ring_table[pick];
        acc = step(acc + axis1_wave9_ring_bias[lane], axis1_wave9_ring_bias[pick]);
    }

    return acc ^ axis1_wave9_ring_bias[seed % 5u];
}
