#include <stdio.h>

typedef struct Axis5W18ProjectionRow {
    unsigned int lane;
    unsigned int watermark;
    unsigned int checkpoint;
    int delta;
} Axis5W18ProjectionRow;

typedef struct Axis5W18ProjectionAgg {
    unsigned int watermark[4];
    unsigned int checkpoint[4];
    int value[4];
} Axis5W18ProjectionAgg;

static unsigned int axis5_w18_projection_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w18_projection_clear(Axis5W18ProjectionAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->watermark[i] = 0u;
        a->checkpoint[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w18_projection_absorb(Axis5W18ProjectionAgg* a, const Axis5W18ProjectionRow* row) {
    unsigned int lane = row->lane % 4u;
    if (row->watermark < a->watermark[lane]) {
        return;
    }
    if (row->watermark > a->watermark[lane]) {
        a->watermark[lane] = row->watermark;
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

static unsigned int axis5_w18_projection_signature(const Axis5W18ProjectionAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w18_projection_mix(h, lane + 1u);
        h = axis5_w18_projection_mix(h, a->watermark[lane]);
        h = axis5_w18_projection_mix(h, a->checkpoint[lane]);
        h = axis5_w18_projection_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W18ProjectionRow rows[] = {
        {0u, 2u, 1u, 5},  {1u, 1u, 4u, 3},  {2u, 3u, 2u, 8},   {3u, 2u, 2u, 7},
        {0u, 4u, 2u, 9},  {1u, 3u, 3u, -1}, {2u, 4u, 3u, 6},  {3u, 5u, 1u, 11},
        {0u, 4u, 5u, -2}, {1u, 3u, 5u, 4},  {2u, 4u, 5u, -3}, {3u, 5u, 4u, 2},
        {0u, 2u, 9u, 99}, {1u, 2u, 7u, 77}, {2u, 1u, 8u, 55}, {3u, 4u, 6u, 44},
        {0u, 4u, 5u, 6},  {1u, 3u, 5u, -2}, {2u, 4u, 5u, 1},  {3u, 5u, 4u, -1}
    };
    Axis5W18ProjectionAgg direct;
    Axis5W18ProjectionAgg projected;

    axis5_w18_projection_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w18_projection_absorb(&direct, &rows[i]);
    }

    axis5_w18_projection_clear(&projected);
    for (unsigned int watermark = 1u; watermark <= 5u; ++watermark) {
        for (unsigned int checkpoint = 5u; checkpoint >= 1u; --checkpoint) {
            for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                if (rows[i].watermark != watermark || rows[i].checkpoint != checkpoint) {
                    continue;
                }
                axis5_w18_projection_absorb(&projected, &rows[i]);
            }
            if (checkpoint == 1u) {
                break;
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w18_projection_signature(&direct);
        unsigned int sig_projected = axis5_w18_projection_signature(&projected);
        unsigned int same = (sig_direct == sig_projected) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_projected, same);
    }
    return 0;
}
