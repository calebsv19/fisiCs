#include <stdio.h>

typedef struct Axis5W14EpochRow {
    unsigned int slot;
    unsigned int checkpoint;
    unsigned int epoch;
    int payload;
} Axis5W14EpochRow;

typedef struct Axis5W14EpochAgg {
    unsigned int checkpoint[4];
    unsigned int epoch[4];
    int payload[4];
} Axis5W14EpochAgg;

static unsigned int axis5_w14_epoch_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w14_epoch_clear(Axis5W14EpochAgg* a) {
    for (int i = 0; i < 4; ++i) {
        a->checkpoint[i] = 0u;
        a->epoch[i] = 0u;
        a->payload[i] = 0;
    }
}

static void axis5_w14_epoch_absorb(
    Axis5W14EpochAgg* a,
    const Axis5W14EpochRow* row
) {
    unsigned int slot = row->slot & 3u;
    if (row->checkpoint < a->checkpoint[slot]) {
        return;
    }
    if (row->checkpoint > a->checkpoint[slot]) {
        a->checkpoint[slot] = row->checkpoint;
        a->epoch[slot] = row->epoch;
        a->payload[slot] = row->payload;
        return;
    }
    if (row->epoch > a->epoch[slot]) {
        a->epoch[slot] = row->epoch;
        a->payload[slot] = row->payload;
    } else if (row->epoch == a->epoch[slot]) {
        a->payload[slot] += row->payload;
    }
}

static unsigned int axis5_w14_epoch_signature(const Axis5W14EpochAgg* a) {
    unsigned int h = 2166136261u;
    for (unsigned int slot = 0; slot < 4u; ++slot) {
        h = axis5_w14_epoch_mix(h, a->checkpoint[slot]);
        h = axis5_w14_epoch_mix(h, a->epoch[slot]);
        h = axis5_w14_epoch_mix(h, (unsigned int)(a->payload[slot] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W14EpochRow rows[] = {
        {0u, 2u, 1u, 5},   {1u, 4u, 2u, 7},   {2u, 3u, 1u, 9},   {3u, 5u, 3u, 4},
        {0u, 2u, 1u, -2},  {1u, 4u, 4u, 3},   {2u, 2u, 8u, 99},  {3u, 5u, 2u, 8},
        {0u, 6u, 2u, 11},  {1u, 3u, 7u, 88},  {2u, 3u, 4u, 6},   {3u, 4u, 9u, 101},
        {0u, 6u, 5u, -1},  {1u, 4u, 4u, -3},  {2u, 3u, 4u, 5},   {3u, 5u, 3u, -2},
        {0u, 5u, 9u, 77},  {1u, 4u, 4u, 6},   {2u, 3u, 2u, 55},  {3u, 5u, 7u, 10},
    };
    Axis5W14EpochAgg direct;
    Axis5W14EpochAgg converged;
    unsigned int best_checkpoint[4] = {0u, 0u, 0u, 0u};

    axis5_w14_epoch_clear(&direct);
    for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        axis5_w14_epoch_absorb(&direct, &rows[i]);
        if (rows[i].checkpoint > best_checkpoint[rows[i].slot & 3u]) {
            best_checkpoint[rows[i].slot & 3u] = rows[i].checkpoint;
        }
    }

    axis5_w14_epoch_clear(&converged);
    for (unsigned int pass = 0; pass < 2u; ++pass) {
        for (unsigned int i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
            if ((i & 1u) == pass &&
                rows[i].checkpoint == best_checkpoint[rows[i].slot & 3u]) {
                axis5_w14_epoch_absorb(&converged, &rows[i]);
            }
        }
    }

    {
        unsigned int sig_direct = axis5_w14_epoch_signature(&direct);
        unsigned int sig_converged = axis5_w14_epoch_signature(&converged);
        unsigned int same = (sig_direct == sig_converged) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_converged, same);
    }
    return 0;
}
