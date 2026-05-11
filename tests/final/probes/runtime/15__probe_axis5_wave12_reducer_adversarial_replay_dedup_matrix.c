#include <stdio.h>

typedef struct Axis5W12Snapshot {
    unsigned int shard;
    unsigned int ticket;
    unsigned int lane;
    unsigned int value;
} Axis5W12Snapshot;

typedef struct Axis5W12State {
    unsigned int ticket_by_shard[3];
    unsigned int lane_by_shard[3];
    unsigned int value_by_shard[3];
} Axis5W12State;

static unsigned int axis5_w12_replay_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w12_replay_clear(Axis5W12State* s) {
    for (int i = 0; i < 3; ++i) {
        s->ticket_by_shard[i] = 0u;
        s->lane_by_shard[i] = 0u;
        s->value_by_shard[i] = 0u;
    }
}

static void axis5_w12_absorb(Axis5W12State* s, const Axis5W12Snapshot* snap) {
    unsigned int shard = snap->shard % 3u;
    if (snap->ticket < s->ticket_by_shard[shard]) {
        return;
    }
    if (snap->ticket == s->ticket_by_shard[shard] &&
        snap->value <= s->value_by_shard[shard]) {
        return;
    }
    s->ticket_by_shard[shard] = snap->ticket;
    s->lane_by_shard[shard] = snap->lane & 3u;
    s->value_by_shard[shard] = snap->value;
}

static unsigned int axis5_w12_replay_signature(const Axis5W12State* s) {
    unsigned int h = 2166136261u;
    for (unsigned int shard = 0; shard < 3u; ++shard) {
        h = axis5_w12_replay_mix(h, shard + 1u);
        h = axis5_w12_replay_mix(h, s->ticket_by_shard[shard]);
        h = axis5_w12_replay_mix(h, s->lane_by_shard[shard]);
        h = axis5_w12_replay_mix(h, s->value_by_shard[shard]);
    }
    return h;
}

int main(void) {
    const Axis5W12Snapshot canonical[] = {
        {0u, 4u, 1u, 17u},
        {1u, 5u, 2u, 33u},
        {2u, 3u, 0u, 21u},
    };
    const Axis5W12Snapshot adversarial[] = {
        {1u, 2u, 2u, 11u},
        {0u, 4u, 1u, 15u},
        {2u, 1u, 0u, 7u},
        {1u, 5u, 2u, 33u},
        {0u, 4u, 1u, 17u},
        {2u, 3u, 0u, 19u},
        {2u, 3u, 0u, 21u},
        {1u, 5u, 2u, 30u},
        {0u, 3u, 1u, 14u},
        {2u, 2u, 0u, 9u},
    };
    Axis5W12State direct;
    Axis5W12State replayed;

    axis5_w12_replay_clear(&direct);
    axis5_w12_replay_clear(&replayed);

    for (int i = 0; i < 3; ++i) {
        axis5_w12_absorb(&direct, &canonical[i]);
    }
    for (int i = 0; i < 10; ++i) {
        axis5_w12_absorb(&replayed, &adversarial[i]);
    }

    {
        unsigned int sig_direct = axis5_w12_replay_signature(&direct);
        unsigned int sig_replayed = axis5_w12_replay_signature(&replayed);
        unsigned int same = (sig_direct == sig_replayed) ? 1u : 0u;
        printf("%u %u %u\n", sig_direct, sig_replayed, same);
    }
    return 0;
}
