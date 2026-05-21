#include <stdio.h>

typedef struct Axis5W16CheckpointRow {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int epoch;
    int delta;
} Axis5W16CheckpointRow;

typedef struct Axis5W16CheckpointAgg {
    unsigned int checkpoint[4];
    unsigned int epoch[4];
    int value[4];
} Axis5W16CheckpointAgg;

static unsigned int axis5_w16_checkpoint_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w16_checkpoint_clear(Axis5W16CheckpointAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->epoch[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w16_checkpoint_absorb(Axis5W16CheckpointAgg* a, const Axis5W16CheckpointRow* row) {
    unsigned int shard = row->shard % 4u;
    if (row->checkpoint < a->checkpoint[shard]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = row->checkpoint;
        a->epoch[shard] = row->epoch;
        a->value[shard] = row->delta;
        return;
    }
    if (row->epoch > a->epoch[shard]) {
        a->epoch[shard] = row->epoch;
        a->value[shard] = row->delta;
    } else if (row->epoch == a->epoch[shard]) {
        a->value[shard] += row->delta;
    }
}

static unsigned int axis5_w16_checkpoint_signature(const Axis5W16CheckpointAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int shard = 0; shard < 4u; ++shard) {
        h = axis5_w16_checkpoint_mix(h, shard + 1u);
        h = axis5_w16_checkpoint_mix(h, a->checkpoint[shard]);
        h = axis5_w16_checkpoint_mix(h, a->epoch[shard]);
        h = axis5_w16_checkpoint_mix(h, (unsigned int)(a->value[shard] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W16CheckpointRow rows[] = {
        {0u, 2u, 1u, 4},   {1u, 1u, 2u, 7},   {2u, 3u, 1u, 6},   {3u, 2u, 4u, 9},
        {0u, 4u, 1u, 8},   {1u, 5u, 2u, 3},   {2u, 4u, 2u, -2},  {3u, 5u, 1u, 10},
        {0u, 4u, 3u, 5},   {1u, 5u, 4u, -1},  {2u, 4u, 3u, 4},   {3u, 5u, 5u, 2},
        {0u, 3u, 9u, 99},  {1u, 4u, 7u, 77},  {2u, 2u, 8u, 55},  {3u, 4u, 6u, 44},
        {0u, 4u, 3u, -2},  {1u, 5u, 4u, 6},   {2u, 4u, 3u, -3},  {3u, 5u, 5u, 1},
    };
    Axis5W16CheckpointAgg direct;
    Axis5W16CheckpointAgg lattice;

    axis5_w16_checkpoint_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w16_checkpoint_absorb(&direct, &rows[i]);
    }

    axis5_w16_checkpoint_clear(&lattice);
    for (unsigned int checkpoint = 5u; checkpoint >= 1u; --checkpoint) {
        for (unsigned int epoch = 1u; epoch <= 5u; ++epoch) {
            for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                if (rows[i].checkpoint != checkpoint || rows[i].epoch != epoch) {
                    continue;
                }
                axis5_w16_checkpoint_absorb(&lattice, &rows[i]);
            }
        }
        if (checkpoint == 1u) {
            break;
        }
    }

    {
        unsigned int sig_direct = axis5_w16_checkpoint_signature(&direct);
        unsigned int sig_lattice = axis5_w16_checkpoint_signature(&lattice);
        unsigned int same = (sig_direct == sig_lattice) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_lattice, same);
    }
    return 0;
}
