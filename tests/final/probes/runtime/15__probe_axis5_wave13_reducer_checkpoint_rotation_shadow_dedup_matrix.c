#include <stdio.h>

typedef struct Axis5W13Checkpoint {
    unsigned int slot;
    unsigned int checkpoint;
    unsigned int frontier;
    unsigned int epoch;
    int value;
} Axis5W13Checkpoint;

typedef struct Axis5W13CheckpointAgg {
    unsigned int checkpoint[3];
    unsigned int frontier[3];
    unsigned int epoch[3];
    int value[3];
} Axis5W13CheckpointAgg;

static unsigned int axis5_w13_checkpoint_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w13_checkpoint_clear(Axis5W13CheckpointAgg* a) {
    for (int i = 0; i < 3; ++i) {
        a->checkpoint[i] = 0u;
        a->frontier[i] = 0u;
        a->epoch[i] = 0u;
        a->value[i] = 0;
    }
}

static void axis5_w13_checkpoint_absorb(
    Axis5W13CheckpointAgg* a,
    const Axis5W13Checkpoint* c
) {
    unsigned int slot = c->slot % 3u;
    if (c->checkpoint < a->checkpoint[slot]) {
        return;
    }
    if (c->checkpoint > a->checkpoint[slot]) {
        a->checkpoint[slot] = c->checkpoint;
        a->frontier[slot] = c->frontier;
        a->epoch[slot] = c->epoch;
        a->value[slot] = c->value;
        return;
    }
    if (c->frontier < a->frontier[slot]) {
        return;
    }
    if (c->frontier > a->frontier[slot]) {
        a->frontier[slot] = c->frontier;
        a->epoch[slot] = c->epoch;
        a->value[slot] = c->value;
        return;
    }
    if (c->epoch > a->epoch[slot]) {
        a->epoch[slot] = c->epoch;
        a->value[slot] = c->value;
    } else if (c->epoch == a->epoch[slot]) {
        a->value[slot] += c->value;
    }
}

static unsigned int axis5_w13_checkpoint_signature(
    const Axis5W13CheckpointAgg* a
) {
    unsigned int h = 2166136261u;
    for (unsigned int slot = 0; slot < 3u; ++slot) {
        h = axis5_w13_checkpoint_mix(h, a->checkpoint[slot]);
        h = axis5_w13_checkpoint_mix(h, a->frontier[slot]);
        h = axis5_w13_checkpoint_mix(h, a->epoch[slot]);
        h = axis5_w13_checkpoint_mix(h, (unsigned int)(a->value[slot] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W13Checkpoint rows[] = {
        {0u, 4u, 2u, 1u, 5},   {1u, 6u, 1u, 2u, 8},   {2u, 5u, 3u, 1u, 7},
        {0u, 4u, 2u, 1u, -3},  {1u, 6u, 4u, 1u, 9},   {2u, 4u, 7u, 3u, 99},
        {0u, 7u, 1u, 5u, 6},   {2u, 5u, 3u, 4u, -2},  {1u, 5u, 9u, 9u, 77},
        {0u, 7u, 3u, 2u, 12},  {1u, 6u, 4u, 3u, -4},  {2u, 5u, 3u, 4u, 10},
        {0u, 6u, 8u, 8u, 100}, {1u, 6u, 3u, 1u, 11},  {2u, 5u, 2u, 6u, 55},
        {0u, 7u, 3u, 4u, -5},  {1u, 6u, 4u, 3u, 6},   {2u, 5u, 3u, 4u, -1},
    };
    Axis5W13CheckpointAgg direct;
    Axis5W13CheckpointAgg rotated;
    unsigned int best_checkpoint[3] = {0u, 0u, 0u};

    axis5_w13_checkpoint_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w13_checkpoint_absorb(&direct, &rows[i]);
        if (rows[i].checkpoint > best_checkpoint[rows[i].slot % 3u]) {
            best_checkpoint[rows[i].slot % 3u] = rows[i].checkpoint;
        }
    }

    axis5_w13_checkpoint_clear(&rotated);
    for (unsigned int pass = 0; pass < 3u; ++pass) {
        for (unsigned int i = pass; i < sizeof(rows) / sizeof(rows[0]); i += 3u) {
            if (rows[i].checkpoint == best_checkpoint[rows[i].slot % 3u]) {
                axis5_w13_checkpoint_absorb(&rotated, &rows[i]);
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w13_checkpoint_signature(&direct);
        unsigned int sig_rotated = axis5_w13_checkpoint_signature(&rotated);
        unsigned int same = (sig_direct == sig_rotated) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_rotated, same);
    }
    return 0;
}
