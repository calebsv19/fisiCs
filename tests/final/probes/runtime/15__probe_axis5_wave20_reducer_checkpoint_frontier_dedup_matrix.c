#include <stdio.h>

typedef struct Axis5W20Row {
    unsigned int lane;
    unsigned int checkpoint;
    unsigned int frontier;
    int delta;
} Axis5W20Row;

typedef struct Axis5W20Agg {
    unsigned int checkpoint[4];
    unsigned int frontier[4];
    int value[4];
} Axis5W20Agg;

static unsigned int axis5_w20_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w20_clear(Axis5W20Agg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->frontier[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w20_absorb(Axis5W20Agg* a, const Axis5W20Row* row) {
    unsigned int lane = row->lane % 4u;
    if (row->checkpoint < a->checkpoint[lane]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[lane]) {
        a->checkpoint[lane] = row->checkpoint;
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

static unsigned int axis5_w20_signature(const Axis5W20Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w20_mix(h, lane + 1u);
        h = axis5_w20_mix(h, a->checkpoint[lane]);
        h = axis5_w20_mix(h, a->frontier[lane]);
        h = axis5_w20_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W20Row rows[] = {
        {0u, 2u, 1u, 5},  {1u, 3u, 2u, 4},  {2u, 2u, 3u, 6},   {3u, 1u, 4u, 3},
        {0u, 5u, 2u, 7},  {1u, 4u, 3u, -2}, {2u, 5u, 3u, 8},   {3u, 4u, 5u, 9},
        {0u, 5u, 2u, -1}, {1u, 4u, 3u, 5},  {2u, 5u, 3u, -3},  {3u, 4u, 5u, -4},
        {0u, 5u, 4u, 2},  {1u, 6u, 4u, 6},  {2u, 5u, 6u, 1},   {3u, 6u, 5u, 4},
        {0u, 4u, 9u, 99}, {1u, 5u, 8u, 77}, {2u, 4u, 7u, 55},  {3u, 5u, 6u, 44},
        {0u, 5u, 4u, 3},  {1u, 6u, 4u, -1}, {2u, 5u, 6u, 5},   {3u, 6u, 5u, -2}
    };
    Axis5W20Agg direct;
    Axis5W20Agg deduped;

    axis5_w20_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w20_absorb(&direct, &rows[i]);
    }

    axis5_w20_clear(&deduped);
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        for (unsigned int checkpoint = 1u; checkpoint <= 6u; ++checkpoint) {
            for (unsigned int frontier = 1u; frontier <= 6u; ++frontier) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 4u != lane || rows[i].checkpoint != checkpoint || rows[i].frontier != frontier) {
                        continue;
                    }
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W20Row merged_row = {lane, checkpoint, frontier, merged};
                    axis5_w20_absorb(&deduped, &merged_row);
                }
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w20_signature(&direct);
        unsigned int sig_deduped = axis5_w20_signature(&deduped);
        unsigned int same = (sig_direct == sig_deduped) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_deduped, same);
    }
    return 0;
}
