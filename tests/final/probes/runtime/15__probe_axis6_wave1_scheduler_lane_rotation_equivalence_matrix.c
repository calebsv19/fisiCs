#include <stdio.h>

typedef struct Axis6W1Row {
    unsigned int lane;
    unsigned int phase;
    unsigned int budget;
    int delta;
} Axis6W1Row;

typedef struct Axis6W1Agg {
    unsigned int phase[4];
    unsigned int budget[4];
    int value[4];
} Axis6W1Agg;

static unsigned int axis6_w1_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis6_w1_clear(Axis6W1Agg* a) {
    for (int i = 0; i < 4; ++i) {
        a->phase[i] = 0u;
        a->budget[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis6_w1_absorb(Axis6W1Agg* a, const Axis6W1Row* row) {
    unsigned int lane = row->lane % 4u;
    if (row->phase < a->phase[lane]) return;
    if (row->phase > a->phase[lane]) {
        a->phase[lane] = row->phase;
        a->budget[lane] = row->budget;
        a->value[lane] = row->delta;
        return;
    }
    if (row->budget > a->budget[lane]) {
        a->budget[lane] = row->budget;
        a->value[lane] = row->delta;
    } else if (row->budget == a->budget[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis6_w1_signature(const Axis6W1Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis6_w1_mix(h, lane + 1u);
        h = axis6_w1_mix(h, a->phase[lane]);
        h = axis6_w1_mix(h, a->budget[lane]);
        h = axis6_w1_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis6W1Row rows[] = {
        {0u,2u,1u,4}, {1u,3u,2u,5}, {2u,1u,3u,7}, {3u,2u,4u,3},
        {0u,5u,2u,6}, {1u,4u,3u,-1}, {2u,5u,2u,8}, {3u,4u,5u,2},
        {0u,5u,2u,-2}, {1u,4u,3u,4}, {2u,5u,4u,1}, {3u,4u,5u,-3},
        {0u,6u,4u,5}, {1u,6u,5u,3}, {2u,6u,4u,6}, {3u,6u,6u,1}
    };
    Axis6W1Agg direct, rotated;
    axis6_w1_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis6_w1_absorb(&direct, &rows[i]);
    }
    axis6_w1_clear(&rotated);
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        for (unsigned int phase = 1u; phase <= 6u; ++phase) {
            for (unsigned int budget = 1u; budget <= 6u; ++budget) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    unsigned int rotated_lane = (rows[i].lane + 1u) % 4u;
                    if (rotated_lane != lane || rows[i].phase != phase || rows[i].budget != budget) continue;
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis6W1Row row = {(lane + 3u) % 4u, phase, budget, merged};
                    axis6_w1_absorb(&rotated, &row);
                }
            }
        }
    }
    {
        unsigned int sig_direct = axis6_w1_signature(&direct);
        unsigned int sig_rotated = axis6_w1_signature(&rotated);
        unsigned int same = (sig_direct == sig_rotated) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_rotated, same);
    }
    return 0;
}
