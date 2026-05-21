#include <stdio.h>

typedef struct Axis5W19FrontierRow {
    unsigned int lane;
    unsigned int frontier;
    unsigned int projection;
    int delta;
} Axis5W19FrontierRow;

typedef struct Axis5W19FrontierAgg {
    unsigned int frontier[5];
    unsigned int projection[5];
    int value[5];
} Axis5W19FrontierAgg;

static unsigned int axis5_w19_frontier_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w19_frontier_clear(Axis5W19FrontierAgg* a) {
    for (int i = 0; i < 5; ++i) {
        a->frontier[i] = 0u;
        a->projection[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w19_frontier_absorb(Axis5W19FrontierAgg* a, const Axis5W19FrontierRow* row) {
    unsigned int lane = row->lane % 5u;
    if (row->frontier < a->frontier[lane]) {
        return;
    }
    if (row->frontier > a->frontier[lane]) {
        a->frontier[lane] = row->frontier;
        a->projection[lane] = row->projection;
        a->value[lane] = row->delta;
        return;
    }
    if (row->projection > a->projection[lane]) {
        a->projection[lane] = row->projection;
        a->value[lane] = row->delta;
    } else if (row->projection == a->projection[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w19_frontier_signature(const Axis5W19FrontierAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w19_frontier_mix(h, lane + 1u);
        h = axis5_w19_frontier_mix(h, a->frontier[lane]);
        h = axis5_w19_frontier_mix(h, a->projection[lane]);
        h = axis5_w19_frontier_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W19FrontierRow rows[] = {
        {0u, 3u, 1u, 5},   {0u, 3u, 1u, -2}, {1u, 2u, 4u, 7},   {1u, 2u, 4u, 1},
        {2u, 4u, 2u, 6},   {2u, 4u, 3u, 2},  {3u, 5u, 1u, 8},   {3u, 5u, 1u, -1},
        {4u, 4u, 2u, 9},   {4u, 4u, 2u, -4}, {0u, 5u, 2u, 4},   {0u, 5u, 2u, 6},
        {1u, 4u, 3u, 3},   {1u, 4u, 3u, -3}, {2u, 4u, 3u, 5},   {2u, 4u, 3u, -1},
        {3u, 5u, 4u, 2},   {3u, 5u, 4u, 7},  {4u, 6u, 3u, 10},  {4u, 6u, 3u, -5}
    };
    Axis5W19FrontierAgg direct;
    Axis5W19FrontierAgg folded;

    axis5_w19_frontier_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w19_frontier_absorb(&direct, &rows[i]);
    }

    axis5_w19_frontier_clear(&folded);
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        for (unsigned int frontier = 1u; frontier <= 6u; ++frontier) {
            for (unsigned int projection = 1u; projection <= 4u; ++projection) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 5u != lane || rows[i].frontier != frontier || rows[i].projection != projection) {
                        continue;
                    }
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W19FrontierRow merged_row = {lane, frontier, projection, merged};
                    axis5_w19_frontier_absorb(&folded, &merged_row);
                }
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w19_frontier_signature(&direct);
        unsigned int sig_folded = axis5_w19_frontier_signature(&folded);
        unsigned int same = (sig_direct == sig_folded) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_folded, same);
    }
    return 0;
}
