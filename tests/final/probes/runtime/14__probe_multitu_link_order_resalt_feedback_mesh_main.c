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

static unsigned run_mesh(const unsigned char* plan, unsigned count, unsigned salt) {
    unsigned h = 2166136261u;
    unsigned x = salt ^ lrf_a_seed() ^ (lrf_b_seed() << 1);
    unsigned i;

    lrf_a_reset(salt + 5u);
    lrf_b_reset(salt + 11u);

    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)plan[i] + i * 3u + 1u;
        if ((plan[i] & 1u) == 0u) {
            x = lrf_a_step(x, lane);
        } else {
            x = lrf_b_step(x, lane + 2u);
        }
        if ((i % 2u) == 0u) {
            x ^= lrf_a_feedback() + i * 33u + salt;
        } else {
            x ^= lrf_b_feedback() + i * 17u + salt * 3u;
        }
        h = fnv1a_u32(h, x ^ lrf_a_feedback());
        h = fnv1a_u32(h, lrf_b_feedback() + lane * 19u + salt);
    }

    return fnv1a_u32(h, x ^ lrf_a_feedback() ^ lrf_b_feedback());
}

int main(void) {
    static const unsigned char plan0[] = {0u, 1u, 3u, 2u, 4u, 1u, 0u, 3u, 2u, 4u};
    static const unsigned char plan1[] = {4u, 2u, 0u, 1u, 3u, 4u, 2u, 1u, 0u, 3u};
    static const unsigned char plan2[] = {1u, 4u, 2u, 0u, 3u, 1u, 4u, 0u, 2u, 3u};
    unsigned h0 = run_mesh(plan0, 10u, 13u);
    unsigned h1 = run_mesh(plan1, 10u, 17u);
    unsigned h2 = run_mesh(plan2, 10u, 23u);
    unsigned repeat = run_mesh(plan1, 10u, 17u);

    if (repeat != h1) {
        printf("link-order-resalt-repeatability-fail %u %u\n", h1, repeat);
        return 41;
    }

    printf("%u %u %u\n", h0, h1, h2);
    return 0;
}
