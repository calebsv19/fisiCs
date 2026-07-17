extern int bucket10_wave60_global_counter;

int bucket10_wave60_lib_step(int step) {
    static int bucket10_wave60_global_counter = 30;

    bucket10_wave60_global_counter += step;
    return bucket10_wave60_global_counter;
}

int bucket10_wave60_lib_total(void) {
    extern int bucket10_wave60_global_counter;

    bucket10_wave60_global_counter += 6;
    return bucket10_wave60_global_counter;
}
