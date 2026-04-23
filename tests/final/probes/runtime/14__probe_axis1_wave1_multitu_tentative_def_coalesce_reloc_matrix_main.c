#include <stdio.h>

extern int axis1_w1_seed;
extern int axis1_w1_table[6];
extern int axis1_w1_mix_lane(int lane, int salt);
extern int axis1_w1_step_counter(int delta);

int axis1_w1_seed;

int main(void) {
    unsigned digest = 2166136261u;
    int i;

    axis1_w1_seed = 9;
    for (i = 0; i < 12; ++i) {
        int mix = axis1_w1_mix_lane(i, 13 + i * 3);
        int step = axis1_w1_step_counter((i & 3) - 1);
        digest ^= (unsigned)(mix + step + axis1_w1_table[(i + 2) % 6]);
        digest *= 16777619u;
    }

    printf("%d %d %u\n", axis1_w1_seed, axis1_w1_table[2], digest);
    return 0;
}
