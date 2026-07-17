int bucket10_wave62_shared[4];

void bucket10_wave62_seed(int base) {
    int i;
    for (i = 0; i < 4; ++i) {
        bucket10_wave62_shared[i] = base + i * 2;
    }
}

int bucket10_wave62_lib_mix(int step) {
    extern int bucket10_wave62_shared[];
    static int bucket10_wave62_local = 30;

    bucket10_wave62_local += step;
    bucket10_wave62_shared[0] += step;
    bucket10_wave62_shared[2] += bucket10_wave62_local / 3;
    {
        int first = bucket10_wave62_local;
        int second = bucket10_wave62_shared[0];
        int bucket10_wave62_shared[2] = {first, second};
        return bucket10_wave62_shared[0] + bucket10_wave62_shared[1] +
               bucket10_wave62_local - 14;
    }
}

int bucket10_wave62_lib_shadow_again(int step) {
    extern int bucket10_wave62_shared[];
    static int bucket10_wave62_local = 33;

    bucket10_wave62_local += step;
    bucket10_wave62_shared[0] += step;
    bucket10_wave62_shared[2] += bucket10_wave62_local / 2;
    return bucket10_wave62_local + bucket10_wave62_shared[0] +
           bucket10_wave62_shared[2];
}
