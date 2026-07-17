#include "10__probe_runtime_wave60_tentative_extern_array_completion.h"

int bucket10_wave60_cells[4] = {2, 4, 6, 8};

int bucket10_wave60_array_seed(int base) {
    int i;

    for (i = 0; i < 4; ++i) {
        bucket10_wave60_cells[i] += base + i;
    }
    return bucket10_wave60_cells[0] + bucket10_wave60_cells[3];
}

int bucket10_wave60_array_fold(void) {
    int sum = 0;
    int i;

    for (i = 0; i < 4; ++i) {
        sum += bucket10_wave60_cells[i] * (i + 1);
    }
    return sum;
}
