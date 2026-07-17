int bucket10_wave64_cells[4];

void bucket10_wave64_seed(int base) {
    int i;
    for (i = 0; i < 4; ++i) {
        bucket10_wave64_cells[i] = base + i;
    }
}

int bucket10_wave64_liba_step(int step) {
    extern int bucket10_wave64_cells[];
    static int local = 7;

    local += step;
    bucket10_wave64_cells[0] += local;
    bucket10_wave64_cells[2] += step;
    {
        int bucket10_wave64_cells[2] = {local, step};
        int external_fold = 33;
        return external_fold +
               bucket10_wave64_cells[0];
    }
}
