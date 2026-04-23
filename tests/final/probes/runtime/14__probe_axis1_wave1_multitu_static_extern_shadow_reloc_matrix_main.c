#include <stdio.h>

extern int axis1_w1_shadow_read_local(int step);
extern int axis1_w1_shadow_mix_external(int ext_value, int step);

int axis1_w1_shadow_token = 7;

int main(void) {
    unsigned digest = 19u;
    int i;

    for (i = 0; i < 10; ++i) {
        int local = axis1_w1_shadow_read_local(i);
        int mix = axis1_w1_shadow_mix_external(axis1_w1_shadow_token, i);
        axis1_w1_shadow_token += (i & 1) ? 3 : 2;
        digest = digest * 181u + (unsigned)(local ^ mix ^ axis1_w1_shadow_token);
    }

    printf("%d %u\n", axis1_w1_shadow_token, digest);
    return 0;
}
