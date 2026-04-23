#include <stdio.h>

unsigned lo_a_seed(void);
unsigned lo_b_seed(void);
void lo_a_reset(void);
void lo_b_reset(void);
unsigned lo_a_step(unsigned x, unsigned lane);
unsigned lo_b_step(unsigned x, unsigned lane);
unsigned lo_a_state(void);
unsigned lo_b_state(void);

static unsigned fnv1a_u32(unsigned h, unsigned v) {
    h ^= (v & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 8) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 16) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 24) & 0xFFu);
    h *= 16777619u;
    return h;
}

static unsigned run_order(const unsigned char* order, unsigned count, unsigned base) {
    unsigned h = 2166136261u;
    unsigned x = base ^ lo_a_seed() ^ (lo_b_seed() << 1);
    unsigned i;

    lo_a_reset();
    lo_b_reset();

    for (i = 0u; i < count; ++i) {
        if ((order[i] & 1u) == 0u) {
            x = lo_a_step(x, i + 3u);
        } else {
            x = lo_b_step(x, i + 7u);
        }
        h = fnv1a_u32(h, x ^ (i * 97u + lo_a_state()));
        h = fnv1a_u32(h, lo_b_state() ^ (i * 131u + 11u));
    }

    return fnv1a_u32(h, x ^ lo_a_state() ^ lo_b_state());
}

int main(void) {
    static const unsigned char order0[] = {0u, 1u, 0u, 1u, 0u, 1u, 0u, 1u, 0u, 1u};
    static const unsigned char order1[] = {1u, 0u, 1u, 1u, 0u, 0u, 1u, 0u, 1u, 0u};
    static const unsigned char order2[] = {0u, 0u, 1u, 1u, 0u, 1u, 1u, 0u, 0u, 1u};
    unsigned h0 = run_order(order0, 10u, 19u);
    unsigned h1 = run_order(order1, 10u, 23u);
    unsigned h2 = run_order(order2, 10u, 29u);
    unsigned repeat = run_order(order1, 10u, 23u);

    if (repeat != h1) {
        printf("repeatability-fail %u %u\n", h1, repeat);
        return 21;
    }

    printf("%u %u %u\n", h0, h1, h2);
    return 0;
}
