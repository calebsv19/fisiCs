#include <stdio.h>

typedef struct Axis5W3Agg {
    unsigned int sum_even;
    unsigned int sum_odd;
    unsigned int xor_lane;
} Axis5W3Agg;

static void axis5_w3_agg_reset(Axis5W3Agg* a) {
    a->sum_even = 0u;
    a->sum_odd = 0u;
    a->xor_lane = 0u;
}

static void axis5_w3_agg_update(Axis5W3Agg* a, unsigned int lane, unsigned int value) {
    if ((lane & 1u) == 0u) a->sum_even += (value & 0xffffu);
    else a->sum_odd += (value & 0xffffu);
    a->xor_lane ^= ((lane & 15u) << 8) ^ (value & 0xffu);
}

static void axis5_w3_agg_merge(Axis5W3Agg* dst, const Axis5W3Agg* src) {
    dst->sum_even += src->sum_even;
    dst->sum_odd += src->sum_odd;
    dst->xor_lane ^= src->xor_lane;
}

static unsigned int axis5_w3_agg_signature(const Axis5W3Agg* a) {
    unsigned int h = 0x811c9dc5u;
    h ^= a->sum_even + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= a->sum_odd + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= a->xor_lane + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

int main(void) {
    const unsigned int lanes[] = {0u, 1u, 3u, 2u, 4u, 5u, 1u, 0u, 7u};
    const unsigned int vals[] = {8u, 3u, 5u, 12u, 7u, 9u, 4u, 11u, 6u};

    Axis5W3Agg full;
    Axis5W3Agg a, b, c, left_merge, right_merge;
    axis5_w3_agg_reset(&full);
    axis5_w3_agg_reset(&a);
    axis5_w3_agg_reset(&b);
    axis5_w3_agg_reset(&c);
    axis5_w3_agg_reset(&left_merge);
    axis5_w3_agg_reset(&right_merge);

    for (int i = 0; i < 9; ++i) axis5_w3_agg_update(&full, lanes[i], vals[i]);
    for (int i = 0; i < 3; ++i) axis5_w3_agg_update(&a, lanes[i], vals[i]);
    for (int i = 3; i < 6; ++i) axis5_w3_agg_update(&b, lanes[i], vals[i]);
    for (int i = 6; i < 9; ++i) axis5_w3_agg_update(&c, lanes[i], vals[i]);

    axis5_w3_agg_merge(&left_merge, &a);
    axis5_w3_agg_merge(&left_merge, &b);
    axis5_w3_agg_merge(&left_merge, &c);

    axis5_w3_agg_merge(&right_merge, &b);
    axis5_w3_agg_merge(&right_merge, &c);
    axis5_w3_agg_merge(&right_merge, &a);

    const unsigned int sig_full = axis5_w3_agg_signature(&full);
    const unsigned int sig_left = axis5_w3_agg_signature(&left_merge);
    const unsigned int sig_right = axis5_w3_agg_signature(&right_merge);
    const unsigned int same = (sig_full == sig_left && sig_left == sig_right) ? 1u : 0u;

    printf("%u %u %u %u\n", sig_full, sig_left, sig_right, same);
    return 0;
}
