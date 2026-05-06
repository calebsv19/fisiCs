#include <stdio.h>

unsigned lrf_a_seed(void);
unsigned lrf_b_seed(void);
void lrf_a_reset(unsigned salt);
void lrf_b_reset(unsigned salt);
unsigned lrf_a_step(unsigned x, unsigned lane);
unsigned lrf_b_step(unsigned x, unsigned lane);
unsigned lrf_a_feedback(void);
unsigned lrf_b_feedback(void);

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

static unsigned run_braid(const unsigned char* plan, unsigned count, unsigned salt) {
    unsigned h = 2166136261u;
    unsigned x = (lrf_a_seed() + salt * 5u) ^ (lrf_b_seed() + salt * 11u);
    unsigned i;

    lrf_a_reset(salt + 13u);
    lrf_b_reset(salt + 19u);

    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)plan[i] + i * 5u + 3u;
        if (((i + plan[i]) & 1u) == 0u) {
            x = lrf_a_step(x ^ lrf_b_feedback(), lane);
        } else {
            x = lrf_b_step(x + lrf_a_feedback(), lane + 4u);
        }
        x ^= lrf_a_feedback() + lrf_b_feedback() + salt * (i + 1u);
        h = fnv1a_u32(h, x);
        h = fnv1a_u32(h, lrf_a_feedback() ^ (lrf_b_feedback() + lane * 23u));
    }

    return fnv1a_u32(h, x ^ lrf_a_feedback() ^ (lrf_b_feedback() << 1));
}

int main(void) {
    static const unsigned char plan0[] = {2u, 0u, 4u, 1u, 3u, 2u, 4u, 0u, 1u, 3u, 2u, 1u};
    static const unsigned char plan1[] = {1u, 3u, 0u, 4u, 2u, 1u, 0u, 3u, 4u, 2u, 1u, 4u};
    static const unsigned char plan2[] = {4u, 2u, 1u, 0u, 3u, 4u, 2u, 0u, 1u, 3u, 4u, 0u};
    unsigned h0 = run_braid(plan0, 12u, 29u);
    unsigned h1 = run_braid(plan1, 12u, 31u);
    unsigned h2 = run_braid(plan2, 12u, 37u);
    unsigned repeat = run_braid(plan1, 12u, 31u);

    if (repeat != h1) {
        printf("link-order-resalt-braid-repeatability-fail %u %u\n", h1, repeat);
        return 47;
    }

    printf("%u %u %u\n", h0, h1, h2);
    return 0;
}
