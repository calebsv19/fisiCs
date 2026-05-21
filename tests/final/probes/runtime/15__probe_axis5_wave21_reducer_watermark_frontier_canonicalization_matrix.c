#include <stdio.h>

typedef struct Axis5W21Row {
    unsigned int lane;
    unsigned int watermark;
    unsigned int frontier;
    int delta;
} Axis5W21Row;

typedef struct Axis5W21Agg {
    unsigned int watermark[4];
    unsigned int frontier[4];
    int value[4];
} Axis5W21Agg;

static unsigned int axis5_w21_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w21_clear(Axis5W21Agg* a) {
    for (int i = 0; i < 4; ++i) {
        a->watermark[i] = 0u;
        a->frontier[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w21_absorb(Axis5W21Agg* a, const Axis5W21Row* row) {
    unsigned int lane = row->lane % 4u;
    if (row->watermark < a->watermark[lane]) {
        return;
    }
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

static unsigned int axis5_w21_signature(const Axis5W21Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w21_mix(h, lane + 1u);
        h = axis5_w21_mix(h, a->watermark[lane]);
        h = axis5_w21_mix(h, a->frontier[lane]);
        h = axis5_w21_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W21Row rows[] = {
        {0u, 2u, 1u, 4},   {1u, 1u, 2u, 7},   {2u, 3u, 1u, 5},   {3u, 2u, 3u, 6},
        {0u, 5u, 2u, 3},   {1u, 4u, 4u, 2},   {2u, 5u, 2u, 8},   {3u, 4u, 5u, -1},
        {0u, 5u, 2u, -2},  {1u, 4u, 4u, 5},   {2u, 5u, 3u, 4},   {3u, 4u, 5u, 7},
        {0u, 5u, 4u, 1},   {1u, 6u, 3u, 9},   {2u, 5u, 3u, -3},  {3u, 6u, 4u, 2},
        {0u, 4u, 9u, 99},  {1u, 5u, 6u, 77},  {2u, 4u, 8u, 55},  {3u, 5u, 7u, 44},
        {0u, 5u, 4u, 6},   {1u, 6u, 3u, -4},  {2u, 5u, 3u, 1},   {3u, 6u, 4u, 3}
    };
    Axis5W21Agg direct;
    Axis5W21Agg canonical;

    axis5_w21_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w21_absorb(&direct, &rows[i]);
    }

    axis5_w21_clear(&canonical);
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        for (unsigned int watermark = 1u; watermark <= 6u; ++watermark) {
            for (unsigned int frontier = 1u; frontier <= 5u; ++frontier) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 4u != lane || rows[i].watermark != watermark || rows[i].frontier != frontier) {
                        continue;
                    }
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W21Row merged_row = {lane, watermark, frontier, merged};
                    axis5_w21_absorb(&canonical, &merged_row);
                }
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w21_signature(&direct);
        unsigned int sig_canonical = axis5_w21_signature(&canonical);
        unsigned int same = (sig_direct == sig_canonical) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_canonical, same);
    }
    return 0;
}
