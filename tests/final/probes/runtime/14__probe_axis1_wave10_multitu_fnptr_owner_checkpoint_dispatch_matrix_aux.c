typedef unsigned (*axis1_wave10_checkpoint_fn)(unsigned, unsigned);

extern axis1_wave10_checkpoint_fn axis1_wave10_checkpoint_table[4];
extern const unsigned axis1_wave10_checkpoint_order[6];
extern const unsigned axis1_wave10_checkpoint_bias[6];

unsigned axis1_wave10_fnptr_owner_checkpoint_fold(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    for (; lane < 6u; ++lane) {
        unsigned pick = axis1_wave10_checkpoint_order[(lane + (seed & 1u)) % 6u] & 3u;
        axis1_wave10_checkpoint_fn step = axis1_wave10_checkpoint_table[pick];
        acc = step(acc + axis1_wave10_checkpoint_bias[lane], axis1_wave10_checkpoint_bias[pick]);
    }

    return acc ^ axis1_wave10_checkpoint_bias[seed % 6u];
}
