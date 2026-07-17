#include <stdio.h>

#include "10__probe_runtime_wave60_tentative_extern_array_completion_current.h"

static int bucket10_wave60_array_current_local(int seed) {
    int local_cells = seed + 9;

    return local_cells + bucket10_wave60_current_cells[0] - 2;
}

int main(void) {
    int local = bucket10_wave60_array_current_local(3);
    int seeded = bucket10_wave60_current_array_seed(5);
    int folded = bucket10_wave60_current_array_fold();

    printf("%d %d %d %d\n", local, seeded, folded, bucket10_wave60_current_cells[2]);
    return 0;
}
