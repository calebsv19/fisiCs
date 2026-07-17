extern int bucket10_wave62_counter;

int bucket10_wave62_aux_touch(int step) {
    static int local = 11;

    local += step;
    bucket10_wave62_counter += step * 3;
    return bucket10_wave62_counter + local;
}
