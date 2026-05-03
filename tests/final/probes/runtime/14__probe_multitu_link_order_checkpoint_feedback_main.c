#include <stdio.h>

unsigned lcf_a_seed(void);
unsigned lcf_b_seed(void);
void lcf_a_reset(unsigned salt);
void lcf_b_reset(unsigned salt);
unsigned lcf_a_step(unsigned x, unsigned lane);
unsigned lcf_b_step(unsigned x, unsigned lane);
unsigned lcf_a_checkpoint(void);
unsigned lcf_b_checkpoint(void);

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

static unsigned run_plan(const unsigned char* order, unsigned count, unsigned salt) {
    unsigned h = 2166136261u;
    unsigned x = salt ^ lcf_a_seed() ^ (lcf_b_seed() << 1);
    unsigned i;

    lcf_a_reset(salt + 3u);
    lcf_b_reset(salt + 7u);

    for (i = 0u; i < count; ++i) {
        if ((order[i] & 1u) == 0u) {
            x = lcf_a_step(x, i + order[i] + 1u);
        } else {
            x = lcf_b_step(x, i + order[i] + 5u);
        }
        if ((i % 3u) == 1u) {
            x ^= lcf_a_checkpoint() + lcf_b_checkpoint() + i * 31u;
        }
        h = fnv1a_u32(h, x ^ lcf_a_checkpoint());
        h = fnv1a_u32(h, lcf_b_checkpoint() + (i * 53u + salt));
    }

    return fnv1a_u32(h, x ^ lcf_a_checkpoint() ^ lcf_b_checkpoint());
}

int main(void) {
    static const unsigned char order0[] = {0u, 1u, 0u, 1u, 1u, 0u, 0u, 1u, 0u};
    static const unsigned char order1[] = {1u, 0u, 1u, 0u, 0u, 1u, 1u, 0u, 1u};
    static const unsigned char order2[] = {0u, 0u, 1u, 1u, 0u, 1u, 0u, 1u, 1u};
    unsigned h0 = run_plan(order0, 9u, 19u);
    unsigned h1 = run_plan(order1, 9u, 23u);
    unsigned h2 = run_plan(order2, 9u, 29u);
    unsigned repeat = run_plan(order1, 9u, 23u);

    if (repeat != h1) {
        printf("link-order-feedback-repeatability-fail %u %u\n", h1, repeat);
        return 37;
    }

    printf("%u %u %u\n", h0, h1, h2);
    return 0;
}
