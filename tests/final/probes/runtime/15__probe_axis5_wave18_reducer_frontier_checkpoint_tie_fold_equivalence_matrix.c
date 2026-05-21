#include <stdio.h>

typedef struct Axis5W18TieRow {
    unsigned int lane;
    unsigned int frontier;
    unsigned int checkpoint;
    int delta;
} Axis5W18TieRow;

typedef struct Axis5W18TieAgg {
    unsigned int frontier[5];
    unsigned int checkpoint[5];
    int value[5];
} Axis5W18TieAgg;

static unsigned int axis5_w18_tie_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w18_tie_clear(Axis5W18TieAgg* a) {
    for (int i = 0; i < 5; ++i) {
        a->frontier[i] = 0u;
        a->checkpoint[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w18_tie_absorb(Axis5W18TieAgg* a, const Axis5W18TieRow* row) {
    unsigned int lane = row->lane % 5u;
    if (row->frontier < a->frontier[lane]) {
        return;
    }
    if (row->frontier > a->frontier[lane]) {
        a->frontier[lane] = row->frontier;
        a->checkpoint[lane] = row->checkpoint;
        a->value[lane] = row->delta;
        return;
    }
    if (row->checkpoint > a->checkpoint[lane]) {
        a->checkpoint[lane] = row->checkpoint;
        a->value[lane] = row->delta;
    } else if (row->checkpoint == a->checkpoint[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w18_tie_signature(const Axis5W18TieAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w18_tie_mix(h, lane + 1u);
        h = axis5_w18_tie_mix(h, a->frontier[lane]);
        h = axis5_w18_tie_mix(h, a->checkpoint[lane]);
        h = axis5_w18_tie_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W18TieRow rows[] = {
        {0u, 3u, 1u, 4},   {0u, 3u, 1u, -1}, {1u, 2u, 4u, 7},   {1u, 2u, 4u, 2},
        {2u, 4u, 2u, 5},   {2u, 4u, 3u, 1},  {3u, 5u, 1u, 8},   {3u, 5u, 1u, -2},
        {4u, 4u, 2u, 6},   {4u, 4u, 2u, 3},  {0u, 5u, 3u, 9},   {0u, 5u, 3u, -4},
        {1u, 4u, 5u, 2},   {1u, 4u, 5u, -2}, {2u, 4u, 4u, 6},   {2u, 4u, 4u, -1},
        {3u, 6u, 2u, 7},   {3u, 6u, 2u, 1},  {4u, 6u, 4u, 10},  {4u, 6u, 4u, -5}
    };
    Axis5W18TieAgg direct;
    Axis5W18TieAgg folded;

    axis5_w18_tie_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w18_tie_absorb(&direct, &rows[i]);
    }

    axis5_w18_tie_clear(&folded);
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        for (unsigned int frontier = 1u; frontier <= 6u; ++frontier) {
            for (unsigned int checkpoint = 1u; checkpoint <= 5u; ++checkpoint) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 5u != lane || rows[i].frontier != frontier || rows[i].checkpoint != checkpoint) {
                        continue;
                    }
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W18TieRow merged_row = {lane, frontier, checkpoint, merged};
                    axis5_w18_tie_absorb(&folded, &merged_row);
                }
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w18_tie_signature(&direct);
        unsigned int sig_folded = axis5_w18_tie_signature(&folded);
        unsigned int same = (sig_direct == sig_folded) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_folded, same);
    }
    return 0;
}
