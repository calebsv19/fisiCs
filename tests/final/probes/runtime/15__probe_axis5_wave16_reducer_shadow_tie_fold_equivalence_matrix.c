#include <stdio.h>

typedef struct Axis5W16ShadowRow {
    unsigned int lane;
    unsigned int checkpoint;
    unsigned int shadow;
    int delta;
} Axis5W16ShadowRow;

typedef struct Axis5W16ShadowAgg {
    unsigned int checkpoint[5];
    unsigned int shadow[5];
    int value[5];
} Axis5W16ShadowAgg;

static unsigned int axis5_w16_shadow_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w16_shadow_clear(Axis5W16ShadowAgg* a) {
    for (int i = 0; i < 5; ++i) {
        a->checkpoint[i] = 0u;
        a->shadow[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w16_shadow_absorb(Axis5W16ShadowAgg* a, const Axis5W16ShadowRow* row) {
    unsigned int lane = row->lane % 5u;
    if (row->checkpoint < a->checkpoint[lane]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[lane]) {
        a->checkpoint[lane] = row->checkpoint;
        a->shadow[lane] = row->shadow;
        a->value[lane] = row->delta;
        return;
    }
    if (row->shadow > a->shadow[lane]) {
        a->shadow[lane] = row->shadow;
        a->value[lane] = row->delta;
    } else if (row->shadow == a->shadow[lane]) {
        a->value[lane] += row->delta;
    }
}

static unsigned int axis5_w16_shadow_signature(const Axis5W16ShadowAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        h = axis5_w16_shadow_mix(h, lane + 1u);
        h = axis5_w16_shadow_mix(h, a->checkpoint[lane]);
        h = axis5_w16_shadow_mix(h, a->shadow[lane]);
        h = axis5_w16_shadow_mix(h, (unsigned int)(a->value[lane] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W16ShadowRow rows[] = {
        {0u, 3u, 1u, 5},   {0u, 3u, 1u, -2},  {1u, 2u, 4u, 7},   {1u, 2u, 4u, 3},
        {2u, 4u, 2u, 6},   {2u, 4u, 3u, 1},   {3u, 5u, 1u, 8},   {3u, 5u, 1u, -1},
        {4u, 4u, 2u, 9},   {4u, 4u, 2u, -3},  {0u, 5u, 2u, 4},   {0u, 5u, 2u, 6},
        {1u, 4u, 3u, 2},   {1u, 4u, 3u, -2},  {2u, 4u, 3u, 5},   {2u, 4u, 3u, -1},
        {3u, 5u, 4u, 3},   {3u, 5u, 4u, 7},   {4u, 6u, 3u, 11},  {4u, 6u, 3u, -4},
    };
    Axis5W16ShadowAgg direct;
    Axis5W16ShadowAgg folded;

    axis5_w16_shadow_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w16_shadow_absorb(&direct, &rows[i]);
    }

    axis5_w16_shadow_clear(&folded);
    for (unsigned int lane = 0; lane < 5u; ++lane) {
        for (unsigned int checkpoint = 1u; checkpoint <= 6u; ++checkpoint) {
            for (unsigned int shadow = 1u; shadow <= 4u; ++shadow) {
                int merged = 0;
                int saw = 0;
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].lane % 5u != lane || rows[i].checkpoint != checkpoint || rows[i].shadow != shadow) {
                        continue;
                    }
                    merged += rows[i].delta;
                    saw = 1;
                }
                if (saw) {
                    Axis5W16ShadowRow merged_row = {lane, checkpoint, shadow, merged};
                    axis5_w16_shadow_absorb(&folded, &merged_row);
                }
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w16_shadow_signature(&direct);
        unsigned int sig_folded = axis5_w16_shadow_signature(&folded);
        unsigned int same = (sig_direct == sig_folded) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_folded, same);
    }
    return 0;
}
