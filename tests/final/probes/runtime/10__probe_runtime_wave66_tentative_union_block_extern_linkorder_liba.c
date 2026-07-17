union bucket10_wave66_union_cell {
    int value;
    int slot[1];
};

union bucket10_wave66_union_cell bucket10_wave66_union_cells[2];

void bucket10_wave66_union_seed(int base) {
    extern union bucket10_wave66_union_cell bucket10_wave66_union_cells[];

    bucket10_wave66_union_cells[0].value = base;
    bucket10_wave66_union_cells[1].slot[0] = base + 1;
}

int bucket10_wave66_union_liba_step(int delta) {
    extern union bucket10_wave66_union_cell bucket10_wave66_union_cells[];

    bucket10_wave66_union_cells[0].slot[0] += delta;
    return bucket10_wave66_union_cells[0].value;
}
