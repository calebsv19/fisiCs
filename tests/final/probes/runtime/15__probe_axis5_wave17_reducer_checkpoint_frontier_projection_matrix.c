#include <stdio.h>

typedef struct Axis5W17CheckpointRow {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int frontier;
    int delta;
} Axis5W17CheckpointRow;

typedef struct Axis5W17CheckpointAgg {
    unsigned int checkpoint[4];
    unsigned int frontier[4];
    int value[4];
} Axis5W17CheckpointAgg;

static unsigned int axis5_w17_checkpoint_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w17_checkpoint_clear(Axis5W17CheckpointAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->frontier[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w17_checkpoint_absorb(Axis5W17CheckpointAgg* a, const Axis5W17CheckpointRow* row) {
    unsigned int shard = row->shard % 4u;
    if (row->checkpoint < a->checkpoint[shard]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = row->checkpoint;
        a->frontier[shard] = row->frontier;
        a->value[shard] = row->delta;
        return;
    }
    if (row->frontier > a->frontier[shard]) {
        a->frontier[shard] = row->frontier;
        a->value[shard] = row->delta;
    } else if (row->frontier == a->frontier[shard]) {
        a->value[shard] += row->delta;
    }
}

static unsigned int axis5_w17_checkpoint_signature(const Axis5W17CheckpointAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int shard = 0; shard < 4u; ++shard) {
        h = axis5_w17_checkpoint_mix(h, shard + 1u);
        h = axis5_w17_checkpoint_mix(h, a->checkpoint[shard]);
        h = axis5_w17_checkpoint_mix(h, a->frontier[shard]);
        h = axis5_w17_checkpoint_mix(h, (unsigned int)(a->value[shard] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W17CheckpointRow rows[] = {
        {0u, 2u, 1u, 4},   {1u, 1u, 2u, 7},   {2u, 3u, 1u, 6},   {3u, 2u, 4u, 9},
        {0u, 4u, 1u, 8},   {1u, 5u, 2u, 3},   {2u, 4u, 2u, -2},  {3u, 5u, 1u, 10},
        {0u, 4u, 3u, 5},   {1u, 5u, 4u, -1},  {2u, 4u, 3u, 4},   {3u, 5u, 5u, 2},
        {0u, 3u, 9u, 99},  {1u, 4u, 7u, 77},  {2u, 2u, 8u, 55},  {3u, 4u, 6u, 44},
        {0u, 4u, 3u, -2},  {1u, 5u, 4u, 6},   {2u, 4u, 3u, -3},  {3u, 5u, 5u, 1}
    };
    Axis5W17CheckpointAgg direct;
    Axis5W17CheckpointAgg projected;

    axis5_w17_checkpoint_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w17_checkpoint_absorb(&direct, &rows[i]);
    }

    axis5_w17_checkpoint_clear(&projected);
    for (unsigned int frontier = 1u; frontier <= 9u; ++frontier) {
        for (unsigned int checkpoint = 5u; checkpoint >= 1u; --checkpoint) {
            for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                if (rows[i].frontier != frontier || rows[i].checkpoint != checkpoint) {
                    continue;
                }
                axis5_w17_checkpoint_absorb(&projected, &rows[i]);
            }
            if (checkpoint == 1u) {
                break;
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w17_checkpoint_signature(&direct);
        unsigned int sig_projected = axis5_w17_checkpoint_signature(&projected);
        unsigned int same = (sig_direct == sig_projected) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_projected, same);
    }
    return 0;
}
