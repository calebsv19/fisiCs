#include <stdio.h>

typedef struct Axis5W13Op {
    unsigned int lane;
    unsigned int frontier;
    unsigned int epoch;
    int delta;
} Axis5W13Op;

typedef struct Axis5W13Agg {
    unsigned int frontier[4];
    unsigned int epoch[4];
    int value[4];
} Axis5W13Agg;

static unsigned int axis5_w13_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w13_clear(Axis5W13Agg* a) {
    for (int i = 0; i < 4; ++i) {
        a->frontier[i] = 0u;
        a->epoch[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w13_apply(Axis5W13Agg* a, const Axis5W13Op* op) {
    unsigned int lane = op->lane & 3u;
    if (op->frontier < a->frontier[lane]) {
        return;
    }
    if (op->frontier > a->frontier[lane]) {
        a->frontier[lane] = op->frontier;
        a->epoch[lane] = op->epoch;
        a->value[lane] = op->delta;
        return;
    }
    if (op->epoch > a->epoch[lane]) {
        a->epoch[lane] = op->epoch;
        a->value[lane] = op->delta;
    } else if (op->epoch == a->epoch[lane]) {
        a->value[lane] += op->delta;
    }
}

static unsigned int axis5_w13_signature(const Axis5W13Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w13_mix(h, lane + 1u);
        h = axis5_w13_mix(h, a->frontier[lane]);
        h = axis5_w13_mix(h, a->epoch[lane]);
        h = axis5_w13_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W13Op ops[] = {
        {0u, 2u, 1u, 5},   {1u, 1u, 2u, 7},   {2u, 3u, 1u, 9},
        {0u, 2u, 1u, -2},  {1u, 4u, 1u, 4},   {2u, 2u, 8u, 12},
        {3u, 1u, 3u, 6},   {1u, 4u, 3u, -1},  {3u, 5u, 2u, 11},
        {0u, 5u, 1u, 14},  {2u, 3u, 1u, -3},  {0u, 4u, 9u, 99},
        {2u, 3u, 4u, 8},   {3u, 5u, 1u, 20},  {0u, 5u, 4u, -5},
        {1u, 3u, 7u, 88},  {2u, 3u, 4u, -2},  {3u, 4u, 8u, 101},
        {1u, 4u, 2u, 3},   {0u, 5u, 4u, 6},   {3u, 5u, 2u, -4},
    };
    Axis5W13Agg direct;
    Axis5W13Agg projected;
    unsigned int latest_frontier[4] = {0u, 0u, 0u, 0u};

    axis5_w13_clear(&direct);
    for (unsigned int i = 0; i < sizeof(ops) / sizeof(ops[0]); ++i) {
        axis5_w13_apply(&direct, &ops[i]);
        if (ops[i].frontier > latest_frontier[ops[i].lane & 3u]) {
            latest_frontier[ops[i].lane & 3u] = ops[i].frontier;
        }
    }

    axis5_w13_clear(&projected);
    for (unsigned int i = 0; i < sizeof(ops) / sizeof(ops[0]); ++i) {
        if (ops[i].frontier == latest_frontier[ops[i].lane & 3u]) {
            axis5_w13_apply(&projected, &ops[i]);
        }
    }

    {
        unsigned int sig_direct = axis5_w13_signature(&direct);
        unsigned int sig_projected = axis5_w13_signature(&projected);
        unsigned int same = (sig_direct == sig_projected) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_projected, same);
    }
    return 0;
}
