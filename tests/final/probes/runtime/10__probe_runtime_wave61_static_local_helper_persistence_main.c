#include <stdio.h>

int bucket10_wave61_counter = 100;

int bucket10_wave61_helper_step(int step);
int bucket10_wave61_helper_global(int step);

static int bucket10_wave61_main_step(int step) {
    static int bucket10_wave61_counter = 5;

    bucket10_wave61_counter += step;
    return bucket10_wave61_counter;
}

int main(void) {
    int a = bucket10_wave61_main_step(4);
    int b = bucket10_wave61_helper_step(7);
    int c = bucket10_wave61_main_step(3);
    int d = bucket10_wave61_helper_step(2);
    int e = bucket10_wave61_helper_global(6);

    printf("%d %d %d %d %d %d\n", a, b, c, d, e, bucket10_wave61_counter);
    return 0;
}
