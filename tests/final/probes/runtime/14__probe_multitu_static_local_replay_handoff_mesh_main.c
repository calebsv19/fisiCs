#include <stdio.h>

unsigned slrhm_seed(void);
unsigned slrhm_step(unsigned lane, unsigned weight);
unsigned slrhm_snapshot(void);

int main(void) {
    static const unsigned lanes[] = {2u, 0u, 3u, 1u, 4u, 2u, 5u, 1u};
    static const unsigned weights[] = {5u, 9u, 14u, 18u, 23u, 27u, 31u, 37u};
    unsigned acc = slrhm_seed();
    unsigned h = 2166136261u;
    unsigned i;

    for (i = 0u; i < 8u; ++i) {
        unsigned step = slrhm_step(lanes[i], weights[i]);
        acc ^= step + (i * 41u + 7u);
        h = h * 16777619u ^ (acc + slrhm_snapshot());
    }

    printf("%u %u\n", h, slrhm_snapshot());
    return 0;
}
