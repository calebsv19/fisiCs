#include <stdio.h>

typedef struct Axis5W11Event {
    unsigned int lane;
    unsigned int epoch;
    int delta;
} Axis5W11Event;

static unsigned int axis5_w11_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w11_filter_before(
    const Axis5W11Event* events,
    int n,
    unsigned int watermark,
    int bins[3]
) {
    for (int i = 0; i < 3; ++i) bins[i] = 0;
    for (int i = 0; i < n; ++i) {
        if (events[i].epoch < watermark) continue;
        bins[events[i].lane % 3u] += events[i].delta;
    }
}

static void axis5_w11_materialize_then_clip(
    const Axis5W11Event* events,
    int n,
    unsigned int watermark,
    int bins[3]
) {
    int per_epoch[3][5];
    for (int lane = 0; lane < 3; ++lane) {
        for (int epoch = 0; epoch < 5; ++epoch) {
            per_epoch[lane][epoch] = 0;
        }
    }
    for (int i = 0; i < n; ++i) {
        unsigned int lane = events[i].lane % 3u;
        unsigned int epoch = events[i].epoch % 5u;
        per_epoch[lane][epoch] += events[i].delta;
    }
    for (int lane = 0; lane < 3; ++lane) {
        bins[lane] = 0;
        for (unsigned int epoch = watermark; epoch < 5u; ++epoch) {
            bins[lane] += per_epoch[lane][epoch];
        }
    }
}

static unsigned int axis5_w11_signature(const int bins[3]) {
    unsigned int h = 2166136261u;
    for (unsigned int i = 0; i < 3u; ++i) {
        h = axis5_w11_mix(h, i + 1u);
        h = axis5_w11_mix(h, (unsigned int)(bins[i] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W11Event events[] = {
        {0u, 0u, +3}, {1u, 1u, +5}, {2u, 2u, +4}, {0u, 3u, +8},
        {1u, 4u, +6}, {2u, 1u, -2}, {0u, 4u, +7}, {2u, 3u, +9},
        {1u, 2u, -3}, {2u, 4u, +2}, {0u, 1u, -1}, {1u, 3u, +4},
    };
    int filtered_bins[3];
    int clipped_bins[3];

    axis5_w11_filter_before(events, 12, 3u, filtered_bins);
    axis5_w11_materialize_then_clip(events, 12, 3u, clipped_bins);

    {
        unsigned int sig_filtered = axis5_w11_signature(filtered_bins);
        unsigned int sig_clipped = axis5_w11_signature(clipped_bins);
        unsigned int same = (sig_filtered == sig_clipped) ? 1u : 0u;
        printf("%u %u %u\n", sig_filtered, sig_clipped, same);
    }
    return 0;
}
