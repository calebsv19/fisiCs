#include <stdio.h>

int bucket10_wave63_cells[3];

void bucket10_wave63_seed(int base);
int bucket10_wave63_liba_step(int step);
int bucket10_wave63_libb_step(int step);
int bucket10_wave63_sum(void);

static int bucket10_wave63_main_step(int step) {
    extern int bucket10_wave63_cells[];
    static int local = 20;

    local += step;
    bucket10_wave63_cells[1] += local;
    {
        int bucket10_wave63_cells[2] = {local, step};
        return bucket10_wave63_cells[0] + bucket10_wave63_cells[1] + step;
    }
}

int main(void) {
    bucket10_wave63_seed(5);
    printf("%d %d %d %d %d %d %d\n",
           bucket10_wave63_liba_step(3),
           bucket10_wave63_main_step(2),
           bucket10_wave63_libb_step(4),
           bucket10_wave63_sum(),
           bucket10_wave63_cells[0],
           bucket10_wave63_cells[1],
           bucket10_wave63_cells[2]);
    return 0;
}
