extern int bucket10_array_lane[4];

int bucket10_array_adjust(int idx, int delta);

void bucket10_array_seed(int base) {
    int i;
    for (i = 0; i < 4; ++i) {
        bucket10_array_lane[i] = base + i;
    }
}

int bucket10_array_mix(int idx, int delta) {
    return bucket10_array_adjust(idx, delta + bucket10_array_lane[0]);
}
