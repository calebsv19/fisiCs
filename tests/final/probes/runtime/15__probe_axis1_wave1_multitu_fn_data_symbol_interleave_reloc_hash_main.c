#include <stdio.h>

int axis1_w1_interleave_step(int lane, int seed);
extern const int axis1_w1_interleave_weights[4];
extern int axis1_w1_interleave_bias_seed;

int main(void) {
    unsigned digest = 2166136261u;
    int i;

    for (i = 0; i < 14; ++i) {
        int out = axis1_w1_interleave_step(i, 17 + i * 5);
        digest ^= (unsigned)(out + axis1_w1_interleave_weights[i & 3] + axis1_w1_interleave_bias_seed);
        digest *= 16777619u;
    }

    printf("%d %u\n", axis1_w1_interleave_bias_seed, digest);
    return 0;
}
