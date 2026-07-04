#include <stdio.h>

typedef struct Axis5W15CFSRow {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int frontier;
    unsigned int shadow;
    int delta;
} Axis5W15CFSRow;

typedef struct Axis5W15CFSAgg {
    unsigned int checkpoint[4];
    unsigned int frontier[4];
    unsigned int shadow[4];
    int value[4];
} Axis5W15CFSAgg;

static unsigned int axis5_w15_cfs_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w15_cfs_clear(Axis5W15CFSAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->frontier[i] = 0u;
        a->shadow[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w15_cfs_absorb(Axis5W15CFSAgg* a, const Axis5W15CFSRow* row) {
    unsigned int shard = row->shard % 4u;
    if (row->checkpoint < a->checkpoint[shard]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = row->checkpoint;
        a->frontier[shard] = row->frontier;
        a->shadow[shard] = row->shadow;
        a->value[shard] = row->delta;
        return;
    }
    if (row->frontier < a->frontier[shard]) {
        return;
    }
    if (row->frontier > a->frontier[shard]) {
        a->frontier[shard] = row->frontier;
        a->shadow[shard] = row->shadow;
        a->value[shard] = row->delta;
        return;
    }
    if (row->shadow > a->shadow[shard]) {
        a->shadow[shard] = row->shadow;
        a->value[shard] = row->delta;
    } else if (row->shadow == a->shadow[shard]) {
        a->value[shard] += row->delta;
    }
}

static unsigned int axis5_w15_cfs_signature(const Axis5W15CFSAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int shard = 0; shard < 4u; ++shard) {
        h = axis5_w15_cfs_mix(h, shard + 1u);
        h = axis5_w15_cfs_mix(h, a->checkpoint[shard]);
        h = axis5_w15_cfs_mix(h, a->frontier[shard]);
        h = axis5_w15_cfs_mix(h, a->shadow[shard]);
        h = axis5_w15_cfs_mix(h, (unsigned int)(a->value[shard] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W15CFSRow rows[] = {
        {0u, 3u, 1u, 1u, 5},   {1u, 2u, 2u, 2u, 7},   {2u, 4u, 1u, 3u, 4},   {3u, 3u, 2u, 1u, 6},
        {0u, 5u, 1u, 2u, 8},   {1u, 4u, 3u, 1u, 2},   {2u, 4u, 2u, 4u, -1},  {3u, 5u, 1u, 3u, 9},
        {0u, 5u, 2u, 1u, -3},  {1u, 4u, 3u, 4u, 5},   {2u, 4u, 2u, 4u, 6},   {3u, 5u, 3u, 2u, 1},
        {0u, 4u, 9u, 9u, 99},  {1u, 3u, 9u, 9u, 88},  {2u, 3u, 8u, 8u, 77},  {3u, 4u, 8u, 8u, 66},
        {0u, 5u, 2u, 3u, 4},   {1u, 4u, 3u, 4u, -2},  {2u, 4u, 2u, 5u, 3},   {3u, 5u, 3u, 2u, -4},
    };
    Axis5W15CFSAgg direct;
    Axis5W15CFSAgg staged;

    axis5_w15_cfs_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w15_cfs_absorb(&direct, &rows[i]);
    }

    axis5_w15_cfs_clear(&staged);
    for (unsigned int checkpoint = 5u; checkpoint >= 1u; --checkpoint) {
        for (unsigned int frontier = 1u; frontier <= 3u; ++frontier) {
            for (unsigned int pass = 0; pass < 2u; ++pass) {
                for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                    if (rows[i].checkpoint != checkpoint || rows[i].frontier != frontier) {
                        continue;
                    }
                    if ((rows[i].shadow % 2u) != pass) {
                        continue;
                    }
                    axis5_w15_cfs_absorb(&staged, &rows[i]);
                }
            }
        }
        if (checkpoint == 1u) {
            break;
        }
    }

    {
        unsigned int sig_direct = axis5_w15_cfs_signature(&direct);
        unsigned int sig_staged = axis5_w15_cfs_signature(&staged);
        printf("%u %u %u\n", sig_direct, sig_staged, (sig_direct == sig_staged) ? 1u : 0u);
    }
    return 0;
}
