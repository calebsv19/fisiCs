union bucket10_wave66_union_cell {
    int value;
    int slot[1];
};

union bucket10_wave66_union_cell bucket10_wave66_union_cells[2];

int bucket10_wave66_union_libb_step(int delta) {
    extern union bucket10_wave66_union_cell bucket10_wave66_union_cells[];

    bucket10_wave66_union_cells[1].value += delta;
    return bucket10_wave66_union_cells[1].slot[0];
}

int bucket10_wave66_union_sum(void) {
    extern union bucket10_wave66_union_cell bucket10_wave66_union_cells[];

    return bucket10_wave66_union_cells[0].value + bucket10_wave66_union_cells[1].value;
}
