extern int bucket10_wave65_alias_cells[];

static int bucket10_wave65_alias_cells_local[3] = {8, 9, 10};

int bucket10_wave65_alias_libb(int step) {
    bucket10_wave65_alias_cells[1] += step;
    bucket10_wave65_alias_cells_local[2] += bucket10_wave65_alias_cells[1];
    {
        int bucket10_wave65_alias_cells[2] = {bucket10_wave65_alias_cells_local[2], step};
        return bucket10_wave65_alias_cells[0] - bucket10_wave65_alias_cells[1];
    }
}

int bucket10_wave65_alias_external_sum(void) {
    extern int bucket10_wave65_alias_cells[];

    return bucket10_wave65_alias_cells[0] + bucket10_wave65_alias_cells[1] +
           bucket10_wave65_alias_cells[2];
}
