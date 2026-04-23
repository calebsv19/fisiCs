#include <stdio.h>

typedef struct Axis5W6State {
    int bins[8];
} Axis5W6State;

typedef struct Axis5W6Op {
    unsigned int key;
    int delta;
} Axis5W6Op;

static unsigned int axis5_w6_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w6_reset(Axis5W6State* s) {
    for (int i = 0; i < 8; ++i) s->bins[i] = 0;
}

static void axis5_w6_apply_eager(Axis5W6State* s, Axis5W6Op op) {
    unsigned int idx = op.key & 7u;
    s->bins[idx] += op.delta;
    if (s->bins[idx] == 0) s->bins[idx] = 0;
}

static void axis5_w6_apply_lazy(Axis5W6State* s, Axis5W6Op op) {
    unsigned int idx = op.key & 7u;
    s->bins[idx] += op.delta;
}

static void axis5_w6_prune_zeros(Axis5W6State* s) {
    for (int i = 0; i < 8; ++i) {
        if (s->bins[i] == 0) s->bins[i] = 0;
    }
}

static unsigned int axis5_w6_sig(const Axis5W6State* s) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 8; ++i) {
        if (s->bins[i] == 0) continue;
        h = axis5_w6_mix(h, (unsigned int)i);
        h = axis5_w6_mix(h, (unsigned int)(s->bins[i] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W6Op ops[] = {
        {1u, +7}, {3u, +5}, {1u, -2}, {4u, +8}, {3u, -5},
        {4u, -3}, {2u, +6}, {1u, -5}, {2u, -2}, {4u, -5},
        {7u, +9}, {2u, -4}, {7u, -9}, {5u, +3}, {5u, -3},
    };
    Axis5W6State eager;
    Axis5W6State lazy;

    axis5_w6_reset(&eager);
    axis5_w6_reset(&lazy);
    for (int i = 0; i < 15; ++i) axis5_w6_apply_eager(&eager, ops[i]);
    for (int i = 0; i < 15; ++i) axis5_w6_apply_lazy(&lazy, ops[i]);
    axis5_w6_prune_zeros(&lazy);

    const unsigned int sig_eager = axis5_w6_sig(&eager);
    const unsigned int sig_lazy = axis5_w6_sig(&lazy);
    const unsigned int same = (sig_eager == sig_lazy) ? 1u : 0u;

    printf("%u %u %u\n", sig_eager, sig_lazy, same);
    return 0;
}
