#include <stdio.h>

typedef struct Axis5W15WETRow {
    unsigned int lane;
    unsigned int watermark;
    unsigned int epoch;
    int delta;
} Axis5W15WETRow;

typedef struct Axis5W15WETAgg {
    unsigned int watermark[5];
    unsigned int epoch[5];
    int value[5];
} Axis5W15WETAgg;

static unsigned int axis5_w15_wet_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w15_wet_clear(Axis5W15WETAgg* a) {
    for (int i = 0; i < 5; ++i) {
        a->watermark[i] = 0u;
        a->epoch[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w15_wet_absorb(Axis5W15WETAgg* a, const Axis5W15WETRow* row) {
    unsigned int lane = row->lane % 5u;
    if (row->watermark < a->watermark[lane]) {
        return;
    }
    if (row->watermark > a->watermark[lane]) {
        a->watermark[lane] = row->watermark;
        a->epoch[lane] = row->epoch;
        a->value[lane] = row->delta;
        return;
    }
    if (row->epoch > a->epoch[lane]) {
        a->epoch[lane] = row->epoch;
        a->value[lane] = row->delta;
    } else if (row->epoch == a->epoch[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w15_wet_signature(const Axis5W15WETAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w15_wet_mix(h, lane + 1u);
        h = axis5_w15_wet_mix(h, a->watermark[lane]);
        h = axis5_w15_wet_mix(h, a->epoch[lane]);
        h = axis5_w15_wet_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W15WETRow rows[] = {
        {0u, 2u, 1u, 3},   {0u, 2u, 1u, -1},  {1u, 3u, 1u, 5},   {1u, 3u, 2u, 7},
        {2u, 4u, 1u, 4},   {2u, 4u, 1u, 4},   {3u, 5u, 2u, 8},   {4u, 1u, 3u, 2},
        {0u, 5u, 2u, 6},   {0u, 5u, 2u, 6},   {1u, 4u, 3u, 2},   {1u, 4u, 3u, -2},
        {2u, 4u, 3u, 9},   {2u, 4u, 3u, -4},  {3u, 5u, 4u, 1},   {3u, 5u, 4u, 1},
        {4u, 6u, 1u, 10},  {4u, 6u, 2u, 5},   {0u, 4u, 9u, 99},  {3u, 4u, 9u, 77},
        {4u, 6u, 2u, -2},  {4u, 6u, 2u, -3},
    };
    Axis5W15WETAgg direct;
    Axis5W15WETAgg folded;

    axis5_w15_wet_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w15_wet_absorb(&direct, &rows[i]);
    }

    axis5_w15_wet_clear(&folded);
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        for (unsigned int watermark = 1u; watermark <= 6u; ++watermark) {
            for (unsigned int epoch = 1u; epoch <= 4u; ++epoch) {
                int delta_sum = 0;
                int matched = 0;
                Axis5W15WETRow folded_row;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 5u != lane || rows[i].watermark != watermark || rows[i].epoch != epoch) {
                        continue;
                    }
                    delta_sum += rows[i].delta;
                    matched = 1;
                }
                if (!matched) {
                    continue;
                }
                folded_row.lane = lane;
                folded_row.watermark = watermark;
                folded_row.epoch = epoch;
                folded_row.delta = delta_sum;
                axis5_w15_wet_absorb(&folded, &folded_row);
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w15_wet_signature(&direct);
        unsigned int sig_folded = axis5_w15_wet_signature(&folded);
        printf("%u %u %u\n", sig_direct, sig_folded, (sig_direct == sig_folded) ? 1u : 0u);
    }
    return 0;
}
