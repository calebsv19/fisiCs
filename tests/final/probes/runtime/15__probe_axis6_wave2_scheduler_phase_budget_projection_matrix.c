#include <stdio.h>

typedef struct Axis6W2Row {
    unsigned int lane;
    unsigned int phase;
    unsigned int budget;
    int delta;
} Axis6W2Row;

typedef struct Axis6W2Agg {
    unsigned int phase[5];
    unsigned int budget[5];
    int value[5];
} Axis6W2Agg;

static unsigned int axis6_w2_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis6_w2_clear(Axis6W2Agg* a) {
    for (int i = 0; i < 5; ++i) {
        a->phase[i] = 0u;
        a->budget[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis6_w2_absorb(Axis6W2Agg* a, const Axis6W2Row* row) {
    unsigned int lane = row->lane % 5u;
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

static unsigned int axis6_w2_signature(const Axis6W2Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis6_w2_mix(h, lane + 1u);
        h = axis6_w2_mix(h, a->phase[lane]);
        h = axis6_w2_mix(h, a->budget[lane]);
        h = axis6_w2_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis6W2Row rows[] = {
        {0u,2u,1u,4}, {1u,3u,2u,5}, {2u,1u,3u,7}, {3u,2u,4u,3}, {4u,1u,2u,6},
        {0u,5u,2u,6}, {1u,4u,3u,-1}, {2u,5u,2u,8}, {3u,4u,5u,2}, {4u,5u,3u,4},
        {0u,5u,2u,-2}, {1u,4u,3u,4}, {2u,5u,4u,1}, {3u,4u,5u,-3}, {4u,5u,3u,-1},
        {0u,6u,4u,5}, {1u,6u,5u,3}, {2u,6u,4u,6}, {3u,6u,6u,1}, {4u,6u,2u,2}
    };
    Axis6W2Agg direct, projected;
    axis6_w2_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) axis6_w2_absorb(&direct, &rows[i]);
    axis6_w2_clear(&projected);
    for (unsigned int phase = 1u; phase <= 6u; ++phase) {
        for (unsigned int lane = 0; lane < 5u; ++lane) {
            for (unsigned int budget = 1u; budget <= 6u; ++budget) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].phase != phase || rows[i].lane % 5u != lane || rows[i].budget != budget) continue;
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis6W2Row row = {lane, phase, budget, merged};
                    axis6_w2_absorb(&projected, &row);
                }
            }
        }
    }
    {
        unsigned int sig_direct = axis6_w2_signature(&direct);
        unsigned int sig_projected = axis6_w2_signature(&projected);
        unsigned int same = (sig_direct == sig_projected) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_projected, same);
    }
    return 0;
}
