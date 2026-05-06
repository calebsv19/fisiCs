#include <stdio.h>

typedef struct Axis5W11Op {
    unsigned int lane;
    unsigned int amount;
} Axis5W11Op;

static unsigned int axis5_w11_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w11_normalize(unsigned int bins[5]) {
    for (int i = 0; i < 4; ++i) {
        unsigned int carry = bins[i] / 10u;
        bins[i] %= 10u;
        bins[i + 1] += carry;
    }
    bins[4] %= 97u;
}

static void axis5_w11_inline_fold(const Axis5W11Op* ops, int n, unsigned int bins[5]) {
    for (int i = 0; i < 5; ++i) bins[i] = 0u;
    for (int i = 0; i < n; ++i) {
        unsigned int lane = ops[i].lane % 4u;
        bins[lane] += ops[i].amount;
        axis5_w11_normalize(bins);
    }
}

static void axis5_w11_post_fold(const Axis5W11Op* ops, int n, unsigned int bins[5]) {
    for (int i = 0; i < 5; ++i) bins[i] = 0u;
    for (int i = 0; i < n; ++i) {
        bins[ops[i].lane % 4u] += ops[i].amount;
    }
    axis5_w11_normalize(bins);
}

static unsigned int axis5_w11_signature(const unsigned int bins[5]) {
    unsigned int h = 2166136261u;
    for (unsigned int i = 0; i < 5u; ++i) {
        h = axis5_w11_mix(h, i + 1u);
        h = axis5_w11_mix(h, bins[i] & 0xffffu);
    }
    return h;
}

int main(void) {
    const Axis5W11Op ops[] = {
        {0u, 17u}, {1u, 8u}, {0u, 6u}, {2u, 31u}, {3u, 9u},
        {1u, 25u}, {2u, 7u}, {0u, 12u}, {3u, 18u}, {1u, 14u},
    };
    unsigned int inline_bins[5];
    unsigned int post_bins[5];

    axis5_w11_inline_fold(ops, 10, inline_bins);
    axis5_w11_post_fold(ops, 10, post_bins);

    {
        unsigned int sig_inline = axis5_w11_signature(inline_bins);
        unsigned int sig_post = axis5_w11_signature(post_bins);
        unsigned int same = (sig_inline == sig_post) ? 1u : 0u;
        printf("%u %u %u\n", sig_inline, sig_post, same);
    }
    return 0;
}
