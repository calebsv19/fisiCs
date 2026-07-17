#include <stdio.h>

int bucket10_wave61_slots[4] = {4, 5, 6, 7};

static int bucket10_wave61_fold(int step) {
    extern int bucket10_wave61_slots[];
    int total = 0;
    int i;

    for (i = 0; i < 4; ++i) {
        bucket10_wave61_slots[i] += step + i;
        total += bucket10_wave61_slots[i];
    }
    {
        int bucket10_wave61_slots[3] = {total, step, total - step};
        total += bucket10_wave61_slots[2] - bucket10_wave61_slots[1];
    }
    return total;
}

int main(void) {
    int a = bucket10_wave61_fold(2);
    int b = bucket10_wave61_fold(1);
    int c = bucket10_wave61_slots[0] + bucket10_wave61_slots[3];

    printf("%d %d %d\n", a, b, c);
    return 0;
}
