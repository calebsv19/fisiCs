extern int bucket10_wave65_alias_cells[];

static int bucket10_wave65_alias_cells_local[3] = {1, 2, 3};

int bucket10_wave65_alias_liba(int step) {
    bucket10_wave65_alias_cells[0] += step;
    bucket10_wave65_alias_cells_local[1] += bucket10_wave65_alias_cells[0];
    {
        int bucket10_wave65_alias_cells[2] = {step, bucket10_wave65_alias_cells_local[1]};
        return bucket10_wave65_alias_cells[0] + bucket10_wave65_alias_cells[1];
    }
}
