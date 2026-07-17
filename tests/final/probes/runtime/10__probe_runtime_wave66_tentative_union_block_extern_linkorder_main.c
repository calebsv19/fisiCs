#include <stdio.h>

union bucket10_wave66_union_cell {
    int value;
    int slot[1];
};

union bucket10_wave66_union_cell bucket10_wave66_union_cells[2];

void bucket10_wave66_union_seed(int base);
int bucket10_wave66_union_liba_step(int delta);
int bucket10_wave66_union_libb_step(int delta);
int bucket10_wave66_union_sum(void);

static int bucket10_wave66_union_main_step(int delta) {
    extern union bucket10_wave66_union_cell bucket10_wave66_union_cells[];

    bucket10_wave66_union_cells[0].value += delta;
    {
        union bucket10_wave66_union_cell bucket10_wave66_union_cells[1];
        bucket10_wave66_union_cells[0].slot[0] = delta + 50;
        return bucket10_wave66_union_cells[0].value;
    }
}

int main(void) {
    bucket10_wave66_union_seed(10);
    printf("%d %d %d %d %d %d\n",
           bucket10_wave66_union_liba_step(3),
           bucket10_wave66_union_main_step(4),
           bucket10_wave66_union_libb_step(2),
           bucket10_wave66_union_sum(),
           bucket10_wave66_union_cells[0].value,
           bucket10_wave66_union_cells[1].value);
    return 0;
}
