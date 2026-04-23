#include <stdio.h>

typedef struct Axis5W8Event {
    unsigned int lane;
    unsigned int value;
    unsigned int tag;
} Axis5W8Event;

static unsigned int axis5_w8_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w8_fold_direct(const Axis5W8Event* ev, int n, unsigned int bins[4]) {
    for (int i = 0; i < 4; ++i) bins[i] = 0u;
    for (int i = 0; i < n; ++i) {
        unsigned int p = ev[i].lane & 3u;
        unsigned int v = (ev[i].value & 0xffu) ^ ((ev[i].tag & 7u) << 2);
        bins[p] += v;
    }
}

static void axis5_w8_fold_via_projection(const Axis5W8Event* ev, int n, unsigned int bins[4]) {
    unsigned int projected_lane[12];
    unsigned int projected_value[12];
    for (int i = 0; i < n; ++i) {
        projected_lane[i] = ev[i].lane & 3u;
        projected_value[i] = (ev[i].value & 0xffu) ^ ((ev[i].tag & 7u) << 2);
    }

    for (int i = 0; i < 4; ++i) bins[i] = 0u;
    for (int i = 0; i < n; ++i) bins[projected_lane[i]] += projected_value[i];
}

static unsigned int axis5_w8_sig(const unsigned int bins[4]) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 4; ++i) h = axis5_w8_mix(h, bins[i]);
    return h;
}

int main(void) {
    const Axis5W8Event stream[] = {
        {0u, 7u, 1u}, {5u, 2u, 4u}, {2u, 9u, 0u}, {7u, 6u, 3u},
        {4u, 5u, 2u}, {1u, 11u, 6u}, {3u, 8u, 1u}, {6u, 4u, 5u},
        {0u, 12u, 7u}, {5u, 10u, 2u}, {2u, 3u, 4u}, {7u, 1u, 0u},
    };
    unsigned int direct[4];
    unsigned int projected[4];

    axis5_w8_fold_direct(stream, 12, direct);
    axis5_w8_fold_via_projection(stream, 12, projected);

    const unsigned int sig_a = axis5_w8_sig(direct);
    const unsigned int sig_b = axis5_w8_sig(projected);
    const unsigned int same = (sig_a == sig_b) ? 1u : 0u;

    printf("%u %u %u\n", sig_a, sig_b, same);
    return 0;
}
