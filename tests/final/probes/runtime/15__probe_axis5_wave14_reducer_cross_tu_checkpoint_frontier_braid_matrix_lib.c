typedef struct Axis5W14Snapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int frontier;
    unsigned int epoch;
    unsigned int lanes[2];
} Axis5W14Snapshot;

typedef struct Axis5W14Aggregate {
    unsigned int checkpoint_by_shard[3];
    unsigned int frontier_by_shard[3];
    unsigned int epoch_by_shard[3];
    unsigned int lanes_by_shard[3][2];
} Axis5W14Aggregate;

static unsigned int axis5_w14_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w14_seed_snapshot(
    Axis5W14Snapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int frontier,
    unsigned int epoch,
    unsigned int a,
    unsigned int b
) {
    out->shard = shard % 3u;
    out->checkpoint = checkpoint;
    out->frontier = frontier;
    out->epoch = epoch;
    out->lanes[0] = a;
    out->lanes[1] = b;
}

void axis5_w14_encode_snapshot(const Axis5W14Snapshot* s, unsigned int wire[6]) {
    wire[0] = s->shard ^ 0x13u;
    wire[1] = s->checkpoint ^ 0x24u;
    wire[2] = s->frontier ^ 0x35u;
    wire[3] = s->epoch ^ 0x46u;
    wire[4] = s->lanes[0] ^ 0x57u;
    wire[5] = s->lanes[1] ^ 0x68u;
}

void axis5_w14_decode_snapshot(Axis5W14Snapshot* s, const unsigned int wire[6]) {
    s->shard = wire[0] ^ 0x13u;
    s->checkpoint = wire[1] ^ 0x24u;
    s->frontier = wire[2] ^ 0x35u;
    s->epoch = wire[3] ^ 0x46u;
    s->lanes[0] = wire[4] ^ 0x57u;
    s->lanes[1] = wire[5] ^ 0x68u;
}

void axis5_w14_clear_aggregate(Axis5W14Aggregate* a) {
    for (int shard = 0; shard < 3; ++shard) {
        a->checkpoint_by_shard[shard] = 0u;
        a->frontier_by_shard[shard] = 0u;
        a->epoch_by_shard[shard] = 0u;
        for (int lane = 0; lane < 2; ++lane) {
            a->lanes_by_shard[shard][lane] = 0u;
        }
    }
}

void axis5_w14_absorb_snapshot(Axis5W14Aggregate* a, const Axis5W14Snapshot* s) {
    unsigned int shard = s->shard % 3u;
    if (s->checkpoint < a->checkpoint_by_shard[shard]) {
        return;
    }
    if (s->checkpoint > a->checkpoint_by_shard[shard]) {
        a->checkpoint_by_shard[shard] = s->checkpoint;
        a->frontier_by_shard[shard] = s->frontier;
        a->epoch_by_shard[shard] = s->epoch;
        a->lanes_by_shard[shard][0] = s->lanes[0];
        a->lanes_by_shard[shard][1] = s->lanes[1];
        return;
    }
    if (s->frontier < a->frontier_by_shard[shard]) {
        return;
    }
    if (s->frontier > a->frontier_by_shard[shard] ||
        s->epoch >= a->epoch_by_shard[shard]) {
        a->frontier_by_shard[shard] = s->frontier;
        a->epoch_by_shard[shard] = s->epoch;
        a->lanes_by_shard[shard][0] = s->lanes[0];
        a->lanes_by_shard[shard][1] = s->lanes[1];
    }
}

unsigned int axis5_w14_snapshot_signature(const Axis5W14Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 3; ++shard) {
        h = axis5_w14_snapshot_mix(h, a->checkpoint_by_shard[shard]);
        h = axis5_w14_snapshot_mix(h, a->frontier_by_shard[shard]);
        h = axis5_w14_snapshot_mix(h, a->epoch_by_shard[shard]);
        h = axis5_w14_snapshot_mix(h, a->lanes_by_shard[shard][0]);
        h = axis5_w14_snapshot_mix(h, a->lanes_by_shard[shard][1]);
    }
    return h;
}
