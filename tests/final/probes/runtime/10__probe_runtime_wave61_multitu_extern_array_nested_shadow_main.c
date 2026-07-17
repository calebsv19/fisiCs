#include <stdio.h>

#include "10__probe_runtime_wave61_multitu_extern_array_nested_shadow.h"

int bucket10_wave61_cells[5] = {2, 4, 6, 8, 10};

static int bucket10_wave61_main_fold(int step) {
    extern int bucket10_wave61_cells[];
    int sum = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        bucket10_wave61_cells[i] += step + i;
        sum += bucket10_wave61_cells[i];
    }
    {
        int bucket10_wave61_cells[2] = {sum - step, step + 9};
        sum += bucket10_wave61_cells[0] - bucket10_wave61_cells[1];
    }
    return sum;
}

int main(void) {
    int a = bucket10_wave61_lib_mix(3);
    int b = bucket10_wave61_main_fold(2);
    int c = bucket10_wave61_lib_static_shadow(4);
    int d = bucket10_wave61_lib_total();

    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
