extern const unsigned axis1_wave4_weights[4];
extern unsigned axis1_wave4_mix(unsigned value);

unsigned axis1_wave4_fold(void) {
    unsigned acc = 0u;
    int i = 0;
    for (; i < 4; ++i) {
        acc = axis1_wave4_mix(acc + axis1_wave4_weights[i]);
    }
    return acc & 0xFFFFu;
}
