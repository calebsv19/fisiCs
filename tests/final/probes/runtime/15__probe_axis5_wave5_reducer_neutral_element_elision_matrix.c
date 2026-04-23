#include <stdio.h>

typedef struct Axis5W5Op {
    unsigned int kind;
    unsigned int key;
    int payload;
} Axis5W5Op;

typedef struct Axis5W5State {
    int bins[8];
} Axis5W5State;

static unsigned int axis5_w5_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w5_reset(Axis5W5State* s) {
    for (int i = 0; i < 8; ++i) s->bins[i] = 0;
}

static void axis5_w5_apply(Axis5W5State* s, Axis5W5Op op) {
    unsigned int idx = op.key & 7u;
    if (op.kind == 3u) return;
    if (op.payload == 0) return;
    if (op.kind == 1u) s->bins[idx] += op.payload;
    if (op.kind == 2u) s->bins[idx] -= op.payload;
}

static void axis5_w5_apply_batch(Axis5W5State* s, const Axis5W5Op* ops, int n) {
    for (int i = 0; i < n; ++i) axis5_w5_apply(s, ops[i]);
}

static unsigned int axis5_w5_sig(const Axis5W5State* s) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 8; ++i) {
        h = axis5_w5_mix(h, (unsigned int)i);
        h = axis5_w5_mix(h, (unsigned int)(s->bins[i] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W5Op baseline[] = {
        {1u, 1u, 7}, {1u, 4u, 3}, {2u, 1u, 2}, {1u, 6u, 8}, {2u, 4u, 1}, {1u, 1u, 5},
    };
    const Axis5W5Op decorated[] = {
        {3u, 7u, 99}, {1u, 1u, 7}, {1u, 5u, 0}, {1u, 4u, 3}, {2u, 1u, 2},
        {3u, 0u, -11}, {1u, 6u, 8}, {2u, 4u, 1}, {2u, 2u, 0}, {1u, 1u, 5}, {3u, 6u, 0},
    };

    Axis5W5State a;
    Axis5W5State b;
    axis5_w5_reset(&a);
    axis5_w5_reset(&b);

    axis5_w5_apply_batch(&a, baseline, 6);
    axis5_w5_apply_batch(&b, decorated, 11);

    const unsigned int sig_a = axis5_w5_sig(&a);
    const unsigned int sig_b = axis5_w5_sig(&b);
    const unsigned int same = (sig_a == sig_b) ? 1u : 0u;

    printf("%u %u %u\n", sig_a, sig_b, same);
    return 0;
}
