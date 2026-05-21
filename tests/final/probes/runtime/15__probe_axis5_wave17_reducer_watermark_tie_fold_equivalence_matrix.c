#include <stdio.h>

typedef struct Axis5W17WatermarkRow {
    unsigned int lane;
    unsigned int frontier;
    unsigned int watermark;
    int delta;
} Axis5W17WatermarkRow;

typedef struct Axis5W17WatermarkAgg {
    unsigned int frontier[5];
    unsigned int watermark[5];
    int value[5];
} Axis5W17WatermarkAgg;

static unsigned int axis5_w17_watermark_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w17_watermark_clear(Axis5W17WatermarkAgg* a) {
    for (int i = 0; i < 5; ++i) {
        a->frontier[i] = 0u;
        a->watermark[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w17_watermark_absorb(Axis5W17WatermarkAgg* a, const Axis5W17WatermarkRow* row) {
    unsigned int lane = row->lane % 5u;
    if (row->frontier < a->frontier[lane]) {
        return;
    }
    if (row->frontier > a->frontier[lane]) {
        a->frontier[lane] = row->frontier;
        a->watermark[lane] = row->watermark;
        a->value[lane] = row->delta;
        return;
    }
    if (row->watermark > a->watermark[lane]) {
        a->watermark[lane] = row->watermark;
        a->value[lane] = row->delta;
    } else if (row->watermark == a->watermark[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w17_watermark_signature(const Axis5W17WatermarkAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w17_watermark_mix(h, lane + 1u);
        h = axis5_w17_watermark_mix(h, a->frontier[lane]);
        h = axis5_w17_watermark_mix(h, a->watermark[lane]);
        h = axis5_w17_watermark_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W17WatermarkRow rows[] = {
        {0u, 3u, 1u, 5},   {0u, 3u, 1u, -2},  {1u, 2u, 4u, 7},   {1u, 2u, 4u, 3},
        {2u, 4u, 2u, 6},   {2u, 4u, 3u, 1},   {3u, 5u, 1u, 8},   {3u, 5u, 1u, -1},
        {4u, 4u, 2u, 9},   {4u, 4u, 2u, -3},  {0u, 5u, 2u, 4},   {0u, 5u, 2u, 6},
        {1u, 4u, 3u, 2},   {1u, 4u, 3u, -2},  {2u, 4u, 3u, 5},   {2u, 4u, 3u, -1},
        {3u, 5u, 4u, 3},   {3u, 5u, 4u, 7},   {4u, 6u, 3u, 11},  {4u, 6u, 3u, -4}
    };
    Axis5W17WatermarkAgg direct;
    Axis5W17WatermarkAgg folded;

    axis5_w17_watermark_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w17_watermark_absorb(&direct, &rows[i]);
    }

    axis5_w17_watermark_clear(&folded);
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        for (unsigned int frontier = 1u; frontier <= 6u; ++frontier) {
            for (unsigned int watermark = 1u; watermark <= 4u; ++watermark) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 5u != lane || rows[i].frontier != frontier || rows[i].watermark != watermark) {
                        continue;
                    }
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W17WatermarkRow merged_row = {lane, frontier, watermark, merged};
                    axis5_w17_watermark_absorb(&folded, &merged_row);
                }
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w17_watermark_signature(&direct);
        unsigned int sig_folded = axis5_w17_watermark_signature(&folded);
        unsigned int same = (sig_direct == sig_folded) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_folded, same);
    }
    return 0;
}
