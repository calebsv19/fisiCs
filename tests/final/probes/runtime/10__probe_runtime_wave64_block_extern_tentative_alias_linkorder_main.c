#include <stdio.h>

int bucket10_wave64_cells[4];

void bucket10_wave64_seed(int base);
int bucket10_wave64_liba_step(int step);
int bucket10_wave64_libb_step(int step);
int bucket10_wave64_sum(void);

static int bucket10_wave64_main_step(int step) {
    extern int bucket10_wave64_cells[];
    static int local = 11;

    local += step;
    bucket10_wave64_cells[1] += local;
    {
        int bucket10_wave64_cells[3] = {local, step, local + step};
        int external_tail = local - step + 7;
        return bucket10_wave64_cells[0] + bucket10_wave64_cells[1] +
               bucket10_wave64_cells[2] + external_tail;
    }
}

int main(void) {
    bucket10_wave64_seed(4);
    printf("%d %d %d %d %d %d %d %d\n",
           bucket10_wave64_liba_step(2),
           bucket10_wave64_main_step(3),
           bucket10_wave64_libb_step(5),
           bucket10_wave64_sum(),
           bucket10_wave64_cells[0],
           bucket10_wave64_cells[1],
           bucket10_wave64_cells[2],
           bucket10_wave64_cells[3]);
    return 0;
}
