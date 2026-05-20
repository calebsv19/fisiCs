#include <stdio.h>

int bucket10_trace_a_seed(int seed);
int bucket10_trace_a_push(int lane, int value);
int bucket10_trace_a_peek(void);

unsigned bucket10_trace_b_seed(unsigned seed);
unsigned bucket10_trace_b_mix(unsigned acc, unsigned lane, unsigned value);
unsigned bucket10_trace_b_peek(void);

static unsigned run_trace(unsigned seed) {
    unsigned acc = bucket10_trace_b_seed(seed * 7u + 1u);
    unsigned lane;
    (void) bucket10_trace_a_seed((int) seed + 3);

    for (lane = 0u; lane < 4u; ++lane) {
        unsigned left = (unsigned) bucket10_trace_a_push((int) lane, (int) (seed + lane * 5u));
        unsigned right = (unsigned) bucket10_trace_a_peek();
        acc = bucket10_trace_b_mix(acc, lane, left + right + lane * 3u);
    }

    return acc ^ bucket10_trace_b_peek() ^ (unsigned) bucket10_trace_a_peek();
}

int main(void) {
    unsigned r1 = run_trace(6u);
    unsigned r2 = run_trace(6u);
    unsigned r3 = run_trace(19u);
    printf("%u %u %u\n", r1, r2, r3);
    if (r1 != r2) {
        return 7;
    }
    return 0;
}
