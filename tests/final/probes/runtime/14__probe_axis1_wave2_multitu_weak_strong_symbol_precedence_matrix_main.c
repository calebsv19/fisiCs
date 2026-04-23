#include <stdio.h>

extern int axis1_w2_ws_symbol_a;
extern int axis1_w2_ws_symbol_b;
extern int axis1_w2_ws_step(int lane, int salt);
extern int axis1_w2_ws_aux_fold(int lane, int salt);

int main(void) {
    unsigned digest = 2166136261u;
    int i;

    for (i = 0; i < 12; ++i) {
        int step = axis1_w2_ws_step(i, 23 + i * 3);
        int fold = axis1_w2_ws_aux_fold(i, 11 + i * 5);
        digest ^= (unsigned)(step + fold + axis1_w2_ws_symbol_a - axis1_w2_ws_symbol_b);
        digest *= 16777619u;
    }

    printf("%d %d %u\n", axis1_w2_ws_symbol_a, axis1_w2_ws_symbol_b, digest);
    return 0;
}
