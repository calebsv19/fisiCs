int bucket10_array_lane[4] = {0, 0, 0, 0};

int bucket10_array_adjust(int idx, int delta) {
    bucket10_array_lane[idx] += delta;
    bucket10_array_lane[2] += idx;
    return bucket10_array_lane[idx] + bucket10_array_lane[2];
}

int bucket10_array_sum(void) {
    int total = 0;
    int i;
    for (i = 0; i < 4; ++i) {
        total += bucket10_array_lane[i];
    }
    return total;
}
