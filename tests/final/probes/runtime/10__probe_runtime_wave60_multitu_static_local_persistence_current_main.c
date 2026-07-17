#include <stdio.h>

int bucket10_wave60_current_global_counter = 20;

int bucket10_wave60_current_lib_step(int step);
int bucket10_wave60_current_lib_total(void);

static int bucket10_wave60_current_main_step(int step) {
    static int bucket10_wave60_current_main_counter = 4;

    bucket10_wave60_current_main_counter += step;
    return bucket10_wave60_current_main_counter;
}

int main(void) {
    int a = bucket10_wave60_current_main_step(3);
    int b = bucket10_wave60_current_lib_step(5);
    int c = bucket10_wave60_current_main_step(2);
    int d = bucket10_wave60_current_lib_total();

    printf("%d %d %d %d %d\n", a, b, c, d, bucket10_wave60_current_global_counter);
    return 0;
}
