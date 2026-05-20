#include <stdio.h>

int bucket10_link_a_reset(int seed);
int bucket10_link_a_step(int lane, int x);
int bucket10_link_a_peek(void);

unsigned bucket10_link_b_reset(unsigned seed);
unsigned bucket10_link_b_mix(unsigned acc, unsigned lane, unsigned value);
unsigned bucket10_link_b_peek(void);

static unsigned run_lane(unsigned seed) {
    unsigned acc = bucket10_link_b_reset(seed * 5u + 3u);
    unsigned lane;
    (void) bucket10_link_a_reset((int) (seed % 17u) + 2);

    for (lane = 0u; lane < 5u; ++lane) {
        unsigned left = (unsigned) bucket10_link_a_step((int) lane, (int) (seed + lane * 9u));
        unsigned right = (unsigned) bucket10_link_a_peek();
        acc = bucket10_link_b_mix(acc, lane, left + right + lane);
    }

    return acc ^ bucket10_link_b_peek() ^ (unsigned) bucket10_link_a_peek();
}

int main(void) {
    unsigned r1 = run_lane(4u);
    unsigned r2 = run_lane(4u);
    unsigned r3 = run_lane(23u);
    printf("%u %u %u\n", r1, r2, r3);
    if (r1 != r2) {
        return 9;
    }
    return 0;
}
