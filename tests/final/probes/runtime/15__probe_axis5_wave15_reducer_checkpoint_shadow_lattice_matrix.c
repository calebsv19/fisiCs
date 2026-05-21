#include <stdio.h>

typedef struct Axis5W15CheckpointRow {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int shadow;
    int delta;
} Axis5W15CheckpointRow;

typedef struct Axis5W15CheckpointAgg {
    unsigned int checkpoint[4];
    unsigned int shadow[4];
    int value[4];
} Axis5W15CheckpointAgg;

static unsigned int axis5_w15_checkpoint_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w15_checkpoint_clear(Axis5W15CheckpointAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->shadow[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w15_checkpoint_absorb(Axis5W15CheckpointAgg* a, const Axis5W15CheckpointRow* row) {
    unsigned int shard = row->shard % 4u;
    if (row->checkpoint < a->checkpoint[shard]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = row->checkpoint;
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

static unsigned int axis5_w15_checkpoint_signature(const Axis5W15CheckpointAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int shard = 0; shard < 4u; ++shard) {
        h = axis5_w15_checkpoint_mix(h, shard + 1u);
        h = axis5_w15_checkpoint_mix(h, a->checkpoint[shard]);
        h = axis5_w15_checkpoint_mix(h, a->shadow[shard]);
        h = axis5_w15_checkpoint_mix(h, (unsigned int)(a->value[shard] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W15CheckpointRow rows[] = {
        {0u, 2u, 1u, 4},   {1u, 1u, 3u, 6},   {2u, 3u, 2u, 5},   {3u, 2u, 2u, 7},
        {0u, 4u, 1u, 8},   {1u, 5u, 2u, 9},   {2u, 4u, 3u, -2},  {3u, 5u, 1u, 10},
        {0u, 4u, 3u, 5},   {1u, 5u, 2u, -4},  {2u, 4u, 3u, 6},   {3u, 5u, 4u, -1},
        {0u, 3u, 9u, 99},  {1u, 4u, 8u, 77},  {2u, 2u, 9u, 55},  {3u, 4u, 8u, 44},
        {0u, 4u, 3u, -2},  {1u, 5u, 5u, 3},   {2u, 4u, 1u, 100}, {3u, 5u, 4u, 2},
    };
    Axis5W15CheckpointAgg direct;
    Axis5W15CheckpointAgg lattice;

    axis5_w15_checkpoint_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w15_checkpoint_absorb(&direct, &rows[i]);
    }

    axis5_w15_checkpoint_clear(&lattice);
    for (unsigned int checkpoint = 5u; checkpoint >= 1u; --checkpoint) {
        for (unsigned int pass = 0; pass < 2u; ++pass) {
            for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                if (rows[i].checkpoint != checkpoint) {
                    continue;
                }
                if ((rows[i].shadow % 2u) != pass) {
                    continue;
                }
                axis5_w15_checkpoint_absorb(&lattice, &rows[i]);
            }
        }
        if (checkpoint == 1u) {
            break;
        }
    }

    {
        unsigned int sig_direct = axis5_w15_checkpoint_signature(&direct);
        unsigned int sig_lattice = axis5_w15_checkpoint_signature(&lattice);
        unsigned int same = (sig_direct == sig_lattice) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_lattice, same);
    }
    return 0;
}
