#include <stdio.h>

typedef struct Axis5W22Row {
    unsigned int lane;
    unsigned int checkpoint;
    unsigned int frontier;
    int delta;
} Axis5W22Row;

typedef struct Axis5W22Agg {
    unsigned int checkpoint[4];
    unsigned int frontier[4];
    int value[4];
} Axis5W22Agg;

static unsigned int axis5_w22_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w22_clear(Axis5W22Agg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->frontier[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w22_absorb(Axis5W22Agg* a, const Axis5W22Row* row) {
    unsigned int lane = row->lane % 4u;
    if (row->checkpoint < a->checkpoint[lane]) return;
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

static unsigned int axis5_w22_signature(const Axis5W22Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w22_mix(h, lane + 1u);
        h = axis5_w22_mix(h, a->checkpoint[lane]);
        h = axis5_w22_mix(h, a->frontier[lane]);
        h = axis5_w22_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W22Row rows[] = {
        {0u,2u,1u,3}, {1u,3u,2u,5}, {2u,1u,3u,7}, {3u,2u,4u,4},
        {0u,5u,2u,6}, {1u,4u,3u,-1}, {2u,5u,2u,8}, {3u,4u,5u,2},
        {0u,5u,2u,-2}, {1u,4u,3u,4}, {2u,5u,4u,1}, {3u,4u,5u,-3},
        {0u,6u,4u,5}, {1u,6u,5u,3}, {2u,6u,4u,6}, {3u,6u,6u,1},
        {0u,6u,4u,2}, {1u,6u,5u,-2}, {2u,6u,4u,-1}, {3u,6u,6u,4}
    };
    Axis5W22Agg direct;
    Axis5W22Agg braided;
    axis5_w22_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i) axis5_w22_absorb(&direct, &rows[i]);
    axis5_w22_clear(&braided);
    for (unsigned int checkpoint = 1u; checkpoint <= 6u; ++checkpoint) {
        for (unsigned int lane = 0; lane < 4u; ++lane) {
            for (unsigned int frontier = 1u; frontier <= 6u; ++frontier) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i) {
                    if (rows[i].checkpoint != checkpoint || rows[i].lane % 4u != lane || rows[i].frontier != frontier) continue;
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W22Row merged_row = {lane, checkpoint, frontier, merged};
                    axis5_w22_absorb(&braided, &merged_row);
                }
            }
        }
    }
    {
        unsigned int sig_direct = axis5_w22_signature(&direct);
        unsigned int sig_braided = axis5_w22_signature(&braided);
        unsigned int same = (sig_direct == sig_braided) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_braided, same);
    }
    return 0;
}
