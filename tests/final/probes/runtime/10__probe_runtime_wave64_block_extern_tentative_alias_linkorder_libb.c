int bucket10_wave64_cells[4];

int bucket10_wave64_libb_step(int step) {
    extern int bucket10_wave64_cells[];
    static int local = 13;

    local -= step;
    bucket10_wave64_cells[0] += step;
    bucket10_wave64_cells[2] += local;
    {
        int bucket10_wave64_cells[2] = {step, local};
        int external_fold = 66;
        return external_fold +
               bucket10_wave64_cells[1];
    }
}

int bucket10_wave64_sum(void) {
    extern int bucket10_wave64_cells[];

    return bucket10_wave64_cells[0] + bucket10_wave64_cells[1] +
           bucket10_wave64_cells[2] + bucket10_wave64_cells[3];
}
