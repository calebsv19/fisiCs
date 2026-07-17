static int bucket10_wave62_counter = 100;

int bucket10_wave62_lib_tick(int step) {
    static int local = 3;

    local += step;
    bucket10_wave62_counter += local;
    return bucket10_wave62_counter + local;
}

int bucket10_wave62_lib_read(void) {
    return bucket10_wave62_counter;
}
