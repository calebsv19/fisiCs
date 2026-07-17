#include <stdio.h>

int bucket10_wave65_alias_cells[3] = {10, 20, 30};

int bucket10_wave65_alias_liba(int step);
int bucket10_wave65_alias_libb(int step);
int bucket10_wave65_alias_external_sum(void);

static int bucket10_wave65_alias_main(int step) {
    extern int bucket10_wave65_alias_cells[];

    bucket10_wave65_alias_cells[2] += step;
    {
        static int bucket10_wave65_alias_cells[2] = {5, 7};
        bucket10_wave65_alias_cells[0] += step;
        bucket10_wave65_alias_cells[1] += bucket10_wave65_alias_cells[0];
        return bucket10_wave65_alias_cells[0] + bucket10_wave65_alias_cells[1];
    }
}

int main(void) {
    printf("%d %d %d %d %d %d %d\n",
           bucket10_wave65_alias_liba(3),
           bucket10_wave65_alias_main(4),
           bucket10_wave65_alias_libb(2),
           bucket10_wave65_alias_external_sum(),
           bucket10_wave65_alias_cells[0],
           bucket10_wave65_alias_cells[1],
           bucket10_wave65_alias_cells[2]);
    return 0;
}
