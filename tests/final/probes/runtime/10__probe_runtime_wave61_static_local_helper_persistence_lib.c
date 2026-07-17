extern int bucket10_wave61_counter;

int bucket10_wave61_helper_step(int step) {
    static int bucket10_wave61_counter = 30;

    bucket10_wave61_counter += step;
    return bucket10_wave61_counter;
}

int bucket10_wave61_helper_global(int step) {
    extern int bucket10_wave61_counter;

    bucket10_wave61_counter += step;
    return bucket10_wave61_counter;
}
