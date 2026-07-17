int bucket10_wave63_cells[3];

int bucket10_wave63_libb_step(int step) {
    extern int bucket10_wave63_cells[];
    static int local = 30;

    local -= step;
    bucket10_wave63_cells[0] += step;
    bucket10_wave63_cells[1] += local / 2;
    return bucket10_wave63_cells[0] + bucket10_wave63_cells[1] +
           bucket10_wave63_cells[2] + local;
}

int bucket10_wave63_sum(void) {
    extern int bucket10_wave63_cells[];

    return bucket10_wave63_cells[0] + bucket10_wave63_cells[1] +
           bucket10_wave63_cells[2];
}
