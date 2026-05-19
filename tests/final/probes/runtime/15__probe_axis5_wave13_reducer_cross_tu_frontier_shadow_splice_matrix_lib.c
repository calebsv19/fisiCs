typedef struct Axis5W13Snapshot {
    unsigned int shard;
    unsigned int frontier;
    unsigned int epoch;
    unsigned int lanes[3];
} Axis5W13Snapshot;

typedef struct Axis5W13Aggregate {
    unsigned int frontier_by_shard[3];
    unsigned int epoch_by_shard[3];
    unsigned int lanes_by_shard[3][3];
} Axis5W13Aggregate;

static unsigned int axis5_w13_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w13_seed_snapshot(
    Axis5W13Snapshot* out,
    unsigned int shard,
    unsigned int frontier,
    unsigned int epoch,
    unsigned int a,
    unsigned int b,
    unsigned int c
) {
    out->shard = shard % 3u;
    out->frontier = frontier;
    out->epoch = epoch;
    out->lanes[0] = a;
    out->lanes[1] = b;
    out->lanes[2] = c;
}

void axis5_w13_encode_snapshot(const Axis5W13Snapshot* s, unsigned int wire[6]) {
    wire[0] = s->shard ^ 0x31u;
    wire[1] = s->frontier ^ 0x42u;
    wire[2] = s->epoch ^ 0x53u;
    wire[3] = s->lanes[0] ^ 0x64u;
    wire[4] = s->lanes[1] ^ 0x75u;
    wire[5] = s->lanes[2] ^ 0x86u;
}

void axis5_w13_decode_snapshot(Axis5W13Snapshot* s, const unsigned int wire[6]) {
    s->shard = wire[0] ^ 0x31u;
    s->frontier = wire[1] ^ 0x42u;
    s->epoch = wire[2] ^ 0x53u;
    s->lanes[0] = wire[3] ^ 0x64u;
    s->lanes[1] = wire[4] ^ 0x75u;
    s->lanes[2] = wire[5] ^ 0x86u;
}

void axis5_w13_clear_aggregate(Axis5W13Aggregate* a) {
    for (int shard = 0; shard < 3; ++shard) {
        a->frontier_by_shard[shard] = 0u;
        a->epoch_by_shard[shard] = 0u;
        for (int lane = 0; lane < 3; ++lane) {
            a->lanes_by_shard[shard][lane] = 0u;
        }
    }
}

void axis5_w13_absorb_snapshot(Axis5W13Aggregate* a, const Axis5W13Snapshot* s) {
    unsigned int shard = s->shard % 3u;
    if (s->frontier < a->frontier_by_shard[shard]) {
        return;
    }
    if (s->frontier == a->frontier_by_shard[shard] &&
        s->epoch < a->epoch_by_shard[shard]) {
        return;
    }
    a->frontier_by_shard[shard] = s->frontier;
    a->epoch_by_shard[shard] = s->epoch;
    for (int lane = 0; lane < 3; ++lane) {
        a->lanes_by_shard[shard][lane] = s->lanes[lane];
    }
}

unsigned int axis5_w13_snapshot_signature(const Axis5W13Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 3; ++shard) {
        h = axis5_w13_snapshot_mix(h, a->frontier_by_shard[shard]);
        h = axis5_w13_snapshot_mix(h, a->epoch_by_shard[shard]);
        for (int lane = 0; lane < 3; ++lane) {
            h = axis5_w13_snapshot_mix(h, a->lanes_by_shard[shard][lane]);
        }
    }
    return h;
}
