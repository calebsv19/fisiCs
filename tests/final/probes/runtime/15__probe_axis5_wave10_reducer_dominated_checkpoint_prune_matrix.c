#include <stdio.h>

typedef struct Axis5W10Checkpoint {
    unsigned int slot;
    unsigned int epoch;
    unsigned int value;
} Axis5W10Checkpoint;

typedef struct Axis5W10State {
    unsigned int epoch[4];
    unsigned int value[4];
} Axis5W10State;

static unsigned int axis5_w10_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w10_clear(Axis5W10State* s) {
    for (int i = 0; i < 4; ++i) {
        s->epoch[i] = 0u;
        s->value[i] = 0u;
    }
}

static void axis5_w10_apply(
    Axis5W10State* s,
    const Axis5W10Checkpoint* cps,
    int n
) {
    axis5_w10_clear(s);
    for (int i = 0; i < n; ++i) {
        unsigned int slot = cps[i].slot & 3u;
        if (cps[i].epoch >= s->epoch[slot]) {
            s->epoch[slot] = cps[i].epoch;
            s->value[slot] = cps[i].value;
        }
    }
}

static int axis5_w10_prune_dominated(
    const Axis5W10Checkpoint* cps,
    int n,
    Axis5W10Checkpoint* pruned
) {
    int out_n = 0;
    for (int i = 0; i < n; ++i) {
        int dominated = 0;
        for (int j = i + 1; j < n; ++j) {
            if ((cps[i].slot & 3u) == (cps[j].slot & 3u) &&
                cps[j].epoch >= cps[i].epoch) {
                dominated = 1;
                break;
            }
        }
        if (!dominated) {
            pruned[out_n++] = cps[i];
        }
    }
    return out_n;
}

static unsigned int axis5_w10_signature(const Axis5W10State* s) {
    unsigned int h = 2166136261u;
    for (unsigned int i = 0; i < 4u; ++i) {
        h = axis5_w10_mix(h, i + 1u);
        h = axis5_w10_mix(h, s->epoch[i] & 0xffffu);
        h = axis5_w10_mix(h, s->value[i] & 0xffffu);
    }
    return h;
}

int main(void) {
    const Axis5W10Checkpoint cps[] = {
        {0u, 1u, 10u},
        {1u, 1u, 4u},
        {0u, 2u, 13u},
        {2u, 1u, 8u},
        {1u, 3u, 12u},
        {3u, 1u, 9u},
        {2u, 2u, 7u},
        {3u, 2u, 15u},
        {1u, 3u, 12u},
        {0u, 2u, 13u},
        {2u, 4u, 11u},
        {3u, 2u, 15u},
    };
    Axis5W10Checkpoint pruned[12];
    Axis5W10State raw_state;
    Axis5W10State pruned_state;

    const int pruned_n = axis5_w10_prune_dominated(cps, 12, pruned);
    axis5_w10_apply(&raw_state, cps, 12);
    axis5_w10_apply(&pruned_state, pruned, pruned_n);

    const unsigned int sig_raw = axis5_w10_signature(&raw_state);
    const unsigned int sig_pruned = axis5_w10_signature(&pruned_state);
    const unsigned int same = (sig_raw == sig_pruned) ? 1u : 0u;

    printf("%u %u %u\n", sig_raw, sig_pruned, same);
    return 0;
}
