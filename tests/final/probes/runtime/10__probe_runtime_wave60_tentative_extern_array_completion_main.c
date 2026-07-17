#include <stdio.h>

#include "10__probe_runtime_wave60_tentative_extern_array_completion.h"

static int bucket10_wave60_array_local(int seed) {
    int bucket10_wave60_cells = seed + 9;
    int local = bucket10_wave60_cells;

    {
        extern int bucket10_wave60_cells[];
        bucket10_wave60_cells[1] += local;
    }

    return local;
}

int main(void) {
    int local = bucket10_wave60_array_local(3);
    int seeded = bucket10_wave60_array_seed(5);
    int folded = bucket10_wave60_array_fold();

    printf("%d %d %d %d\n", local, seeded, folded, bucket10_wave60_cells[2]);
    return 0;
}
