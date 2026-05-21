#include <stdio.h>

typedef struct Axis5W24Row {
    unsigned int lane;
    unsigned int watermark;
    unsigned int frontier;
    int delta;
} Axis5W24Row;

typedef struct Axis5W24Agg {
    unsigned int watermark[5];
    unsigned int frontier[5];
    int value[5];
} Axis5W24Agg;

static unsigned int axis5_w24_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w24_clear(Axis5W24Agg* a) {
    for (int i = 0; i < 5; ++i) {
        a->watermark[i] = 0u;
        a->frontier[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w24_absorb(Axis5W24Agg* a, const Axis5W24Row* row) {
    unsigned int lane = row->lane % 5u;
    if (row->watermark < a->watermark[lane]) return;
    if (row->watermark > a->watermark[lane]) {
        a->watermark[lane] = row->watermark;
        a->frontier[lane] = row->frontier;
        a->value[lane] = row->delta;
        return;
    }
    if (row->frontier > a->frontier[lane]) {
        a->frontier[lane] = row->frontier;
        a->value[lane] = row->delta;
    } else if (row->frontier == a->frontier[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w24_signature(const Axis5W24Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w24_mix(h, lane + 1u);
        h = axis5_w24_mix(h, a->watermark[lane]);
        h = axis5_w24_mix(h, a->frontier[lane]);
        h = axis5_w24_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W24Row rows[] = {
        {0u,2u,1u,4}, {1u,3u,2u,5}, {2u,1u,3u,7}, {3u,2u,4u,3}, {4u,2u,1u,6},
        {0u,5u,2u,6}, {1u,4u,3u,-1}, {2u,5u,2u,8}, {3u,4u,5u,2}, {4u,5u,3u,4},
        {0u,5u,2u,-2}, {1u,4u,3u,4}, {2u,5u,4u,1}, {3u,4u,5u,-3}, {4u,5u,3u,-1},
        {0u,6u,4u,5}, {1u,6u,5u,3}, {2u,6u,4u,6}, {3u,6u,6u,1}, {4u,6u,2u,2},
        {0u,6u,4u,2}, {1u,6u,5u,-2}, {2u,6u,4u,-1}, {3u,6u,6u,4}, {4u,6u,2u,5}
    };
    Axis5W24Agg direct, collapsed;
    axis5_w24_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w24_absorb(&direct, &rows[i]);
    }
    axis5_w24_clear(&collapsed);
    for (unsigned int watermark = 1u; watermark <= 6u; ++watermark) {
        for (unsigned int lane = 0; lane < 5u; ++lane) {
            for (unsigned int frontier = 1u; frontier <= 6u; ++frontier) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].watermark != watermark || rows[i].lane % 5u != lane || rows[i].frontier != frontier) {
                        continue;
                    }
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W24Row merged_row = {lane, watermark, frontier, merged};
                    axis5_w24_absorb(&collapsed, &merged_row);
                }
            }
        }
    }
    {
        unsigned int sig_direct = axis5_w24_signature(&direct);
        unsigned int sig_collapsed = axis5_w24_signature(&collapsed);
        unsigned int same = (sig_direct == sig_collapsed) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_collapsed, same);
    }
    return 0;
}
