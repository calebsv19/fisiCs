#include <stddef.h>

typedef struct Axis5W15CESSnapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int epoch;
    unsigned int shadow;
    int lane_sum;
} Axis5W15CESSnapshot;

typedef struct Axis5W15CESAggregate {
    unsigned int checkpoint[3];
    unsigned int epoch[3];
    unsigned int shadow[3];
    int lane_sum[3];
} Axis5W15CESAggregate;

static unsigned int axis5_w15_ces_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w15_ces_seed_snapshot(
    Axis5W15CESSnapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int epoch,
    unsigned int shadow,
    int lane_sum
) {
    out->shard = shard % 3u;
    out->checkpoint = checkpoint;
    out->epoch = epoch;
    out->shadow = shadow;
    out->lane_sum = lane_sum;
}

void axis5_w15_ces_encode_snapshot(const Axis5W15CESSnapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard;
    wire[1] = s->checkpoint;
    wire[2] = s->epoch;
    wire[3] = s->shadow;
    wire[4] = (unsigned int)(s->lane_sum + 4096);
}

void axis5_w15_ces_decode_snapshot(Axis5W15CESSnapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] % 3u;
    s->checkpoint = wire[1];
    s->epoch = wire[2];
    s->shadow = wire[3];
    s->lane_sum = (int)wire[4] - 4096;
}

void axis5_w15_ces_clear_aggregate(Axis5W15CESAggregate* a) {
    for (int i = 0; i < 3; ++i) {
        a->checkpoint[i] = 0u;
        a->epoch[i] = 0u;
        a->shadow[i] = 0u;
        a->lane_sum[i] = 0;
    }
}

void axis5_w15_ces_absorb_snapshot(Axis5W15CESAggregate* a, const Axis5W15CESSnapshot* s) {
    unsigned int shard = s->shard % 3u;
    if (s->checkpoint < a->checkpoint[shard]) {
        return;
    }
    if (s->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = s->checkpoint;
        a->epoch[shard] = s->epoch;
        a->shadow[shard] = s->shadow;
        a->lane_sum[shard] = s->lane_sum;
        return;
    }
    if (s->shadow > a->shadow[shard]) {
        a->epoch[shard] = s->epoch;
        a->shadow[shard] = s->shadow;
        a->lane_sum[shard] = s->lane_sum;
        return;
    }
    if (s->shadow == a->shadow[shard]) {
        if (s->epoch > a->epoch[shard]) {
            a->epoch[shard] = s->epoch;
            a->lane_sum[shard] = s->lane_sum;
        } else if (s->epoch == a->epoch[shard]) {
            a->lane_sum[shard] += s->lane_sum;
        }
    }
}

unsigned int axis5_w15_ces_signature(const Axis5W15CESAggregate* a) {
    unsigned int h = 2166136261u;
    for (unsigned int shard = 0; shard < 3u; ++shard) {
        h = axis5_w15_ces_mix(h, shard + 1u);
        h = axis5_w15_ces_mix(h, a->checkpoint[shard]);
        h = axis5_w15_ces_mix(h, a->epoch[shard]);
        h = axis5_w15_ces_mix(h, a->shadow[shard]);
        h = axis5_w15_ces_mix(h, (unsigned int)(a->lane_sum[shard] & 0xffff));
    }
    return h;
}
