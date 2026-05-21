#include <stdio.h>

typedef struct Axis5W19Row {
    unsigned int lane;
    unsigned int checkpoint;
    unsigned int watermark;
    int delta;
} Axis5W19Row;

typedef struct Axis5W19Agg {
    unsigned int checkpoint[4];
    unsigned int watermark[4];
    int value[4];
} Axis5W19Agg;

static unsigned int axis5_w19_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w19_clear(Axis5W19Agg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->watermark[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w19_absorb(Axis5W19Agg* a, const Axis5W19Row* row) {
    unsigned int lane = row->lane % 4u;
    if (row->checkpoint < a->checkpoint[lane]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[lane]) {
        a->checkpoint[lane] = row->checkpoint;
        a->watermark[lane] = row->watermark;
        a->value[lane] = row->delta;
        return;
    }
    if (row->watermark > a->watermark[lane]) {
        a->watermark[lane] = row->watermark;
        a->value[lane] = row->delta;
    } else if (row->watermark == a->watermark[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w19_signature(const Axis5W19Agg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 4u; ++lane) {
        h = axis5_w19_mix(h, lane + 1u);
        h = axis5_w19_mix(h, a->checkpoint[lane]);
        h = axis5_w19_mix(h, a->watermark[lane]);
        h = axis5_w19_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W19Row rows[] = {
        {0u, 2u, 1u, 3},   {1u, 3u, 2u, 7},   {2u, 1u, 4u, 5},   {3u, 2u, 3u, 9},
        {0u, 4u, 2u, 6},   {1u, 5u, 3u, -1},  {2u, 4u, 5u, 8},  {3u, 5u, 4u, 2},
        {0u, 4u, 4u, 5},   {1u, 5u, 5u, 3},   {2u, 4u, 5u, -2}, {3u, 5u, 4u, 4},
        {0u, 3u, 9u, 99},  {1u, 4u, 7u, 77},  {2u, 2u, 8u, 55}, {3u, 4u, 6u, 44},
        {0u, 4u, 4u, -1},  {1u, 5u, 5u, 6},   {2u, 4u, 5u, 1},  {3u, 5u, 4u, -3}
    };
    Axis5W19Agg direct;
    Axis5W19Agg replayed;

    axis5_w19_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w19_absorb(&direct, &rows[i]);
    }

    axis5_w19_clear(&replayed);
    for (unsigned int watermark = 1u; watermark <= 9u; ++watermark) {
        for (unsigned int checkpoint = 5u; checkpoint >= 1u; --checkpoint) {
            for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                if (rows[i].watermark != watermark || rows[i].checkpoint != checkpoint) {
                    continue;
                }
                axis5_w19_absorb(&replayed, &rows[i]);
            }
            if (checkpoint == 1u) {
                break;
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w19_signature(&direct);
        unsigned int sig_replayed = axis5_w19_signature(&replayed);
        unsigned int same = (sig_direct == sig_replayed) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_replayed, same);
    }
    return 0;
}
