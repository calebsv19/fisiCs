int bucket10_wave63_cells[3];

void bucket10_wave63_seed(int base) {
    int i;
    for (i = 0; i < 3; ++i) {
        bucket10_wave63_cells[i] = base + i;
    }
}

int bucket10_wave63_liba_step(int step) {
    extern int bucket10_wave63_cells[];
    static int local = 10;

    local += step;
    bucket10_wave63_cells[0] += local;
    bucket10_wave63_cells[2] += step;
    return bucket10_wave63_cells[0] * 2 + bucket10_wave63_cells[1] +
           bucket10_wave63_cells[2] + local;
}
