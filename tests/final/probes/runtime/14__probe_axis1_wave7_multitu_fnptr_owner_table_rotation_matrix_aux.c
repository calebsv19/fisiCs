typedef unsigned (*axis1_wave7_mix_fn)(unsigned, unsigned);

extern axis1_wave7_mix_fn axis1_wave7_mix_table[4];
extern const unsigned axis1_wave7_owner_perm[4];
extern const unsigned axis1_wave7_owner_bias[4];

unsigned axis1_wave7_fnptr_owner_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    for (; lane < 4u; ++lane) {
        unsigned idx = axis1_wave7_owner_perm[(lane + (seed & 1u)) & 3u];
        axis1_wave7_mix_fn step = axis1_wave7_mix_table[idx];
        acc = step(acc + axis1_wave7_owner_bias[lane], axis1_wave7_owner_bias[idx]);
    }

    return acc ^ axis1_wave7_owner_bias[seed & 3u];
}
