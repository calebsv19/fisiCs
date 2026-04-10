#include <stdio.h>

int wave93_a_reset(int seed);
int wave93_a_step(int lane, int x);
int wave93_a_peek(void);

unsigned wave93_b_reset(unsigned seed);
unsigned wave93_b_mix(unsigned acc, unsigned lane, unsigned value);
unsigned wave93_b_snapshot(void);

static unsigned run_lane(unsigned seed) {
    unsigned acc = wave93_b_reset(seed * 3u + 11u);
    unsigned lane;
    (void) wave93_a_reset((int) (seed % 101u) - 37);

    for (lane = 0u; lane < 8u; ++lane) {
        unsigned left = (unsigned) wave93_a_step((int) lane, (int) (seed + lane * 19u));
        unsigned right = (unsigned) wave93_a_peek();
        acc = wave93_b_mix(acc, lane, left + right + lane * 7u);
    }

    return acc ^ wave93_b_snapshot() ^ (unsigned) wave93_a_peek();
}

int main(void) {
    unsigned r1 = run_lane(17u);
    unsigned r2 = run_lane(17u);
    unsigned r3 = run_lane(221u);
    printf("%u %u %u\n", r1, r2, r3);
    if (r1 != r2) {
        return 9;
    }
    return 0;
}
