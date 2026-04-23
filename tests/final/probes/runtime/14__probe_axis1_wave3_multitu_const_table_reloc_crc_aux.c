extern unsigned axis1_wave3_fetch(int index);

unsigned axis1_wave3_fold(unsigned seed) {
    int i;
    for (i = 0; i < 6; ++i) {
        seed = (seed * 33u) + axis1_wave3_fetch(i);
    }
    return seed;
}
