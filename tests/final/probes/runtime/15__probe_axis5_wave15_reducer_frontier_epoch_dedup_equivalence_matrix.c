#include <stdio.h>

typedef struct Axis5W15FrontierRow {
    unsigned int lane;
    unsigned int frontier;
    unsigned int epoch;
    int delta;
} Axis5W15FrontierRow;

typedef struct Axis5W15FrontierAgg {
    unsigned int frontier[5];
    unsigned int epoch[5];
    int value[5];
} Axis5W15FrontierAgg;

static unsigned int axis5_w15_frontier_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w15_frontier_clear(Axis5W15FrontierAgg* a) {
    for (int i = 0; i < 5; ++i) {
        a->frontier[i] = 0u;
        a->epoch[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w15_frontier_absorb(Axis5W15FrontierAgg* a, const Axis5W15FrontierRow* row) {
    unsigned int lane = row->lane % 5u;
    if (row->frontier < a->frontier[lane]) {
        return;
    }
    if (row->frontier > a->frontier[lane]) {
        a->frontier[lane] = row->frontier;
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

static unsigned int axis5_w15_frontier_signature(const Axis5W15FrontierAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w15_frontier_mix(h, lane + 1u);
        h = axis5_w15_frontier_mix(h, a->frontier[lane]);
        h = axis5_w15_frontier_mix(h, a->epoch[lane]);
        h = axis5_w15_frontier_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W15FrontierRow rows[] = {
        {0u, 3u, 1u, 4},   {0u, 3u, 1u, 4},   {1u, 2u, 3u, 7},   {1u, 2u, 3u, 7},
        {2u, 4u, 2u, 6},   {2u, 4u, 2u, -1},  {3u, 5u, 1u, 8},   {4u, 1u, 4u, 3},
        {0u, 4u, 2u, 5},   {0u, 4u, 2u, 5},   {1u, 5u, 2u, 9},   {1u, 5u, 2u, -4},
        {2u, 4u, 3u, 2},   {2u, 4u, 3u, 2},   {3u, 5u, 4u, 1},   {3u, 5u, 4u, 1},
        {4u, 6u, 1u, 10},  {4u, 6u, 1u, -2},  {0u, 2u, 9u, 99},  {3u, 4u, 9u, 77},
        {4u, 6u, 2u, 3},   {4u, 6u, 2u, 3},
    };
    Axis5W15FrontierAgg direct;
    Axis5W15FrontierAgg deduped;

    axis5_w15_frontier_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w15_frontier_absorb(&direct, &rows[i]);
    }

    axis5_w15_frontier_clear(&deduped);
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        for (unsigned int frontier = 1u; frontier <= 6u; ++frontier) {
            for (unsigned int epoch = 1u; epoch <= 4u; ++epoch) {
                int seen = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 5u != lane || rows[i].frontier != frontier || rows[i].epoch != epoch) {
                        continue;
                    }
                    if (seen) {
                        continue;
                    }
                    axis5_w15_frontier_absorb(&deduped, &rows[i]);
                    seen = 1;
                }
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w15_frontier_signature(&direct);
        unsigned int sig_deduped = axis5_w15_frontier_signature(&deduped);
        unsigned int same = (sig_direct == sig_deduped) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_deduped, same);
    }
    return 0;
}
