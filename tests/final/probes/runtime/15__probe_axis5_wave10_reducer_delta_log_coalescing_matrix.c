#include <stdio.h>

typedef struct Axis5W10Op {
    unsigned int key;
    int delta;
} Axis5W10Op;

static unsigned int axis5_w10_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w10_fold_bins(const Axis5W10Op* ops, int n, int bins[6]) {
    for (int i = 0; i < 6; ++i) {
        bins[i] = 0;
    }
    for (int i = 0; i < n; ++i) {
        bins[ops[i].key % 6u] += ops[i].delta;
    }
}

static int axis5_w10_coalesce_adjacent(
    const Axis5W10Op* ops,
    int n,
    Axis5W10Op* compact
) {
    int out_n = 0;
    for (int i = 0; i < n; ++i) {
        if (out_n > 0 && compact[out_n - 1].key == ops[i].key) {
            compact[out_n - 1].delta += ops[i].delta;
        } else {
            compact[out_n++] = ops[i];
        }
    }
    return out_n;
}

static unsigned int axis5_w10_signature(const int bins[6]) {
    unsigned int h = 2166136261u;
    for (unsigned int i = 0; i < 6u; ++i) {
        if (bins[i] == 0) {
            continue;
        }
        h = axis5_w10_mix(h, i + 1u);
        h = axis5_w10_mix(h, (unsigned int)(bins[i] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W10Op raw_ops[] = {
        {0u, +5}, {0u, -2}, {0u, +1},
        {3u, +7}, {3u, -3},
        {1u, +4}, {1u, +2},
        {5u, +6},
        {2u, -1}, {2u, +1},
        {4u, +9}, {4u, -4}, {4u, -2},
        {5u, -3}, {5u, +3},
    };
    Axis5W10Op compact[15];
    int raw_bins[6];
    int compact_bins[6];

    const int compact_n = axis5_w10_coalesce_adjacent(raw_ops, 15, compact);
    axis5_w10_fold_bins(raw_ops, 15, raw_bins);
    axis5_w10_fold_bins(compact, compact_n, compact_bins);

    const unsigned int sig_raw = axis5_w10_signature(raw_bins);
    const unsigned int sig_compact = axis5_w10_signature(compact_bins);
    const unsigned int same = (sig_raw == sig_compact) ? 1u : 0u;

    printf("%u %u %u\n", sig_raw, sig_compact, same);
    return 0;
}
