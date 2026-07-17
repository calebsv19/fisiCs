extern int bucket10_wave60_current_global_counter;

int bucket10_wave60_current_lib_step(int step) {
    static int bucket10_wave60_current_lib_counter = 30;

    bucket10_wave60_current_lib_counter += step;
    return bucket10_wave60_current_lib_counter;
}

int bucket10_wave60_current_lib_total(void) {
    bucket10_wave60_current_global_counter += 6;
    return bucket10_wave60_current_global_counter;
}
