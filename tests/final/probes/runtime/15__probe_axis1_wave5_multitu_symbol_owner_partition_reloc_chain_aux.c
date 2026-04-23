extern unsigned (*axis1_wave5_mix_ptr)(unsigned);
extern const unsigned axis1_wave5_owner_weights[3];

unsigned axis1_wave5_owner_fold(unsigned seed) {
    unsigned acc = seed;
    int i = 0;
    for (; i < 3; ++i) {
        acc = axis1_wave5_mix_ptr(acc + axis1_wave5_owner_weights[i]);
    }
    return acc;
}
