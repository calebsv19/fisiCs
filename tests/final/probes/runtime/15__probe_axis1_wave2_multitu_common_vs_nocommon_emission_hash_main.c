#include <stdio.h>

extern int axis1_w2_common_counter;
extern int axis1_w2_common_only;
extern int axis1_w2_common_tick(int lane, int salt);
extern int axis1_w2_common_aux_shift(int lane, int salt);

int main(void) {
    unsigned digest = 2166136261u;
    int i;

    for (i = 0; i < 13; ++i) {
        int tick = axis1_w2_common_tick(i, 29 + i * 4);
        int shift = axis1_w2_common_aux_shift(i, 3 + i);
        digest ^= (unsigned)(tick + shift + axis1_w2_common_counter + axis1_w2_common_only);
        digest *= 16777619u;
    }

    printf("%d %d %u\n", axis1_w2_common_counter, axis1_w2_common_only, digest);
    return 0;
}
