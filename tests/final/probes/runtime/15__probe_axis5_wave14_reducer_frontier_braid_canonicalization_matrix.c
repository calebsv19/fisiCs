#include <stdio.h>

typedef struct Axis5W14Row {
    unsigned int lane;
    unsigned int frontier;
    unsigned int epoch;
    int delta;
} Axis5W14Row;

typedef struct Axis5W14Agg {
    unsigned int frontier[5];
    unsigned int epoch[5];
    int value[5];
} Axis5W14Agg;

static unsigned int axis5_w14_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w14_clear(Axis5W14Agg* a) {
    for (int i = 0; i < 5; ++i) {
        a->frontier[i] = 0u;
        a->epoch[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w14_absorb(Axis5W14Agg* a, const Axis5W14Row* row) {
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

static unsigned int axis5_w14_signature(const Axis5W14Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w14_mix(h, lane + 1u);
        h = axis5_w14_mix(h, a->frontier[lane]);
        h = axis5_w14_mix(h, a->epoch[lane]);
        h = axis5_w14_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W14Row rows[] = {
        {0u, 2u, 1u, 5},   {1u, 3u, 2u, 7},   {2u, 1u, 4u, 9},
        {3u, 4u, 1u, 6},   {4u, 2u, 3u, 8},   {0u, 5u, 2u, -1},
        {1u, 3u, 2u, -3},  {2u, 4u, 1u, 4},   {3u, 4u, 3u, 2},
        {4u, 5u, 1u, 11},  {0u, 5u, 2u, 6},   {2u, 4u, 3u, -2},
        {1u, 2u, 9u, 99},  {3u, 5u, 2u, 12},  {4u, 5u, 1u, -4},
        {0u, 4u, 8u, 77},  {1u, 3u, 4u, 5},   {2u, 4u, 3u, 10},
        {3u, 5u, 2u, -3},  {4u, 5u, 4u, 7},
    };
    Axis5W14Agg direct;
    Axis5W14Agg braided;
    unsigned int max_frontier[5] = {0u, 0u, 0u, 0u, 0u};

    axis5_w14_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w14_absorb(&direct, &rows[i]);
        if (rows[i].frontier > max_frontier[rows[i].lane % 5u]) {
            max_frontier[rows[i].lane % 5u] = rows[i].frontier;
        }
    }

    axis5_w14_clear(&braided);
    for (unsigned int phase = 0; phase < 3u; ++phase) {
        for (unsigned int i = phase; i < sizeof(rows) / sizeof(rows[0]); i += 3u) {
            if (rows[i].frontier == max_frontier[rows[i].lane % 5u]) {
                axis5_w14_absorb(&braided, &rows[i]);
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w14_signature(&direct);
        unsigned int sig_braided = axis5_w14_signature(&braided);
        unsigned int same = (sig_direct == sig_braided) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_braided, same);
    }
    return 0;
}
