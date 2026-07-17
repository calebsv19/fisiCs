#include "10__probe_runtime_wave61_multitu_extern_array_nested_shadow.h"

int bucket10_wave61_lib_mix(int step) {
    extern int bucket10_wave61_cells[];
    int sum = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        bucket10_wave61_cells[i] += step;
        sum += bucket10_wave61_cells[i];
    }
    return sum;
}

int bucket10_wave61_lib_static_shadow(int step) {
    static int bucket10_wave61_cells = 17;

    bucket10_wave61_cells += step;
    return bucket10_wave61_cells;
}

int bucket10_wave61_lib_total(void) {
    return bucket10_wave61_cells[0] + bucket10_wave61_cells[1] +
           bucket10_wave61_cells[2] + bucket10_wave61_cells[3] +
           bucket10_wave61_cells[4];
}
