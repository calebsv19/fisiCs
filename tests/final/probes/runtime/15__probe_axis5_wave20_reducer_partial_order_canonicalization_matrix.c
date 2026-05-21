#include <stdio.h>

typedef struct Axis5W20PartialRow {
    unsigned int lane;
    unsigned int epoch;
    unsigned int checkpoint;
    int delta;
} Axis5W20PartialRow;

typedef struct Axis5W20PartialAgg {
    unsigned int epoch[5];
    unsigned int checkpoint[5];
    int value[5];
} Axis5W20PartialAgg;

static unsigned int axis5_w20_partial_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w20_partial_clear(Axis5W20PartialAgg* a) {
    for (int i = 0; i < 5; ++i) {
        a->epoch[i] = 0u;
        a->checkpoint[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w20_partial_absorb(Axis5W20PartialAgg* a, const Axis5W20PartialRow* row) {
    unsigned int lane = row->lane % 5u;
    if (row->epoch < a->epoch[lane]) {
        return;
    }
    if (row->epoch > a->epoch[lane]) {
        a->epoch[lane] = row->epoch;
        a->checkpoint[lane] = row->checkpoint;
        a->value[lane] = row->delta;
        return;
    }
    if (row->checkpoint > a->checkpoint[lane]) {
        a->checkpoint[lane] = row->checkpoint;
        a->value[lane] = row->delta;
    } else if (row->checkpoint == a->checkpoint[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w20_partial_signature(const Axis5W20PartialAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w20_partial_mix(h, lane + 1u);
        h = axis5_w20_partial_mix(h, a->epoch[lane]);
        h = axis5_w20_partial_mix(h, a->checkpoint[lane]);
        h = axis5_w20_partial_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

static void axis5_w20_partial_absorb_batch(
    Axis5W20PartialAgg* agg,
    const Axis5W20PartialRow* rows,
    unsigned int count
) {
    for (unsigned int i = 0; i < count; ++i) {
        axis5_w20_partial_absorb(agg, &rows[i]);
    }
}

int main(void) {
    const Axis5W20PartialRow shard_a[] = {
        {0u, 2u, 1u, 4}, {1u, 3u, 1u, 7}, {2u, 2u, 2u, 5}, {3u, 1u, 1u, 6}
    };
    const Axis5W20PartialRow shard_b[] = {
        {0u, 4u, 2u, 3}, {1u, 3u, 4u, -2}, {2u, 5u, 1u, 8}, {4u, 2u, 3u, 9}
    };
    const Axis5W20PartialRow shard_c[] = {
        {0u, 4u, 2u, 6}, {1u, 6u, 2u, 5}, {3u, 4u, 4u, 2}, {4u, 5u, 1u, 4}
    };
    const Axis5W20PartialRow shard_d[] = {
        {2u, 5u, 3u, -1}, {3u, 4u, 4u, 7}, {4u, 5u, 1u, -2}, {1u, 6u, 2u, 1}
    };
    Axis5W20PartialAgg direct;
    Axis5W20PartialAgg merge_left;
    Axis5W20PartialAgg merge_right;

    axis5_w20_partial_clear(&direct);
    axis5_w20_partial_absorb_batch(&direct, shard_a, sizeof(shard_a) / sizeof(shard_a[0]));
    axis5_w20_partial_absorb_batch(&direct, shard_b, sizeof(shard_b) / sizeof(shard_b[0]));
    axis5_w20_partial_absorb_batch(&direct, shard_c, sizeof(shard_c) / sizeof(shard_c[0]));
    axis5_w20_partial_absorb_batch(&direct, shard_d, sizeof(shard_d) / sizeof(shard_d[0]));

    axis5_w20_partial_clear(&merge_left);
    axis5_w20_partial_absorb_batch(&merge_left, shard_a, sizeof(shard_a) / sizeof(shard_a[0]));
    axis5_w20_partial_absorb_batch(&merge_left, shard_b, sizeof(shard_b) / sizeof(shard_b[0]));
    axis5_w20_partial_absorb_batch(&merge_left, shard_c, sizeof(shard_c) / sizeof(shard_c[0]));
    axis5_w20_partial_absorb_batch(&merge_left, shard_d, sizeof(shard_d) / sizeof(shard_d[0]));

    axis5_w20_partial_clear(&merge_right);
    axis5_w20_partial_absorb_batch(&merge_right, shard_c, sizeof(shard_c) / sizeof(shard_c[0]));
    axis5_w20_partial_absorb_batch(&merge_right, shard_a, sizeof(shard_a) / sizeof(shard_a[0]));
    axis5_w20_partial_absorb_batch(&merge_right, shard_d, sizeof(shard_d) / sizeof(shard_d[0]));
    axis5_w20_partial_absorb_batch(&merge_right, shard_b, sizeof(shard_b) / sizeof(shard_b[0]));

    {
        unsigned int sig_direct = axis5_w20_partial_signature(&direct);
        unsigned int sig_left = axis5_w20_partial_signature(&merge_left);
        unsigned int sig_right = axis5_w20_partial_signature(&merge_right);
        unsigned int same = (sig_direct == sig_left && sig_direct == sig_right) ? 1u : 0u;
        printf("%u %u %u %u\n", sig_direct, sig_left, sig_right, same);
    }
    return 0;
}
