#include <stdio.h>

typedef struct Axis5W12Op {
    unsigned int lane;
    unsigned int epoch;
    int delta;
} Axis5W12Op;

typedef struct Axis5W12Agg {
    unsigned int epoch[4];
    int value[4];
} Axis5W12Agg;

static unsigned int axis5_w12_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w12_clear(Axis5W12Agg* a) {
    for (int i = 0; i < 4; ++i) {
        a->epoch[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w12_apply(Axis5W12Agg* a, const Axis5W12Op* ops, int n) {
    for (int i = 0; i < n; ++i) {
        unsigned int lane = ops[i].lane & 3u;
        if (ops[i].epoch > a->epoch[lane]) {
            a->epoch[lane] = ops[i].epoch;
            a->value[lane] = ops[i].delta;
        } else if (ops[i].epoch == a->epoch[lane]) {
            a->value[lane] += ops[i].delta;
        }
    }
}

static void axis5_w12_merge(Axis5W12Agg* dst, const Axis5W12Agg* src) {
    for (int lane = 0; lane < 4; ++lane) {
        if (src->epoch[lane] > dst->epoch[lane]) {
            dst->epoch[lane] = src->epoch[lane];
            dst->value[lane] = src->value[lane];
        } else if (src->epoch[lane] == dst->epoch[lane]) {
            dst->value[lane] += src->value[lane];
        }
    }
}

static unsigned int axis5_w12_signature(const Axis5W12Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w12_mix(h, lane + 1u);
        h = axis5_w12_mix(h, a->epoch[lane]);
        h = axis5_w12_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W12Op shard_a[] = {{0u, 2u, 4}, {1u, 1u, 7}, {2u, 3u, 5}};
    const Axis5W12Op shard_b[] = {{0u, 2u, -1}, {1u, 4u, 9}, {3u, 2u, 6}};
    const Axis5W12Op shard_c[] = {{2u, 3u, -2}, {3u, 5u, 8}, {1u, 4u, -3}};
    Axis5W12Agg direct;
    Axis5W12Agg a;
    Axis5W12Agg b;
    Axis5W12Agg c;
    Axis5W12Agg merge_left;
    Axis5W12Agg merge_right;

    axis5_w12_clear(&direct);
    axis5_w12_apply(&direct, shard_a, 3);
    axis5_w12_apply(&direct, shard_b, 3);
    axis5_w12_apply(&direct, shard_c, 3);

    axis5_w12_clear(&a);
    axis5_w12_clear(&b);
    axis5_w12_clear(&c);
    axis5_w12_apply(&a, shard_a, 3);
    axis5_w12_apply(&b, shard_b, 3);
    axis5_w12_apply(&c, shard_c, 3);

    merge_left = a;
    axis5_w12_merge(&merge_left, &b);
    axis5_w12_merge(&merge_left, &c);

    merge_right = b;
    axis5_w12_merge(&merge_right, &c);
    axis5_w12_merge(&merge_right, &a);

    {
        unsigned int sig_direct = axis5_w12_signature(&direct);
        unsigned int sig_left = axis5_w12_signature(&merge_left);
        unsigned int sig_right = axis5_w12_signature(&merge_right);
        unsigned int same = (sig_direct == sig_left && sig_left == sig_right) ? 1u : 0u;
        printf("%u %u %u %u\n", sig_direct, sig_left, sig_right, same);
    }
    return 0;
}
