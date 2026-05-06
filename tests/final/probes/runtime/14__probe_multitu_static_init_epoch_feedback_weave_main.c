#include <stdio.h>

unsigned sief_a_boot(void);
unsigned sief_b_boot(void);
void sief_a_reset(unsigned salt);
void sief_b_reset(unsigned salt);
unsigned sief_a_emit(unsigned lane);
unsigned sief_b_emit(unsigned lane);
unsigned sief_a_epoch(void);
unsigned sief_b_epoch(void);

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

static unsigned run_weave(const unsigned char* plan, unsigned count, unsigned salt) {
    unsigned h = 2166136261u;
    unsigned x = sief_a_boot() ^ (sief_b_boot() << 1) ^ (salt * 17u);
    unsigned i;

    sief_a_reset(salt + 5u);
    sief_b_reset(salt + 9u);

    for (i = 0u; i < count; ++i) {
        unsigned lane = (unsigned)plan[i] + i * 2u + 1u;
        if ((plan[i] & 1u) == 0u) {
            x += sief_a_emit(lane);
        } else {
            x ^= sief_b_emit(lane + 5u);
        }
        if ((i % 2u) == 0u) {
            x ^= sief_a_epoch() + (sief_b_epoch() << 2);
        } else {
            x ^= sief_b_epoch() + (sief_a_epoch() << 2);
        }
        h = fnv1a_u32(h, x ^ lane * 29u);
        h = fnv1a_u32(h, sief_a_epoch() + sief_b_epoch() + salt * (i + 3u));
    }

    return fnv1a_u32(h, x ^ sief_a_epoch() ^ (sief_b_epoch() << 1));
}

int main(void) {
    static const unsigned char plan0[] = {3u, 1u, 4u, 0u, 2u, 3u, 1u, 4u, 2u, 0u, 3u};
    static const unsigned char plan1[] = {0u, 2u, 4u, 1u, 3u, 0u, 2u, 4u, 1u, 3u, 2u};
    static const unsigned char plan2[] = {4u, 3u, 1u, 2u, 0u, 4u, 3u, 1u, 0u, 2u, 4u};
    unsigned h0 = run_weave(plan0, 11u, 41u);
    unsigned h1 = run_weave(plan1, 11u, 43u);
    unsigned h2 = run_weave(plan2, 11u, 47u);
    unsigned repeat = run_weave(plan1, 11u, 43u);

    if (repeat != h1) {
        printf("static-init-epoch-weave-repeatability-fail %u %u\n", h1, repeat);
        return 53;
    }

    printf("%u %u %u\n", h0, h1, h2);
    return 0;
}
