typedef struct Axis5W18Snapshot {
    unsigned int shard;
    unsigned int frontier;
    unsigned int checkpoint;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W18Snapshot;

typedef struct Axis5W18Aggregate {
    unsigned int frontier[3];
    unsigned int checkpoint[3];
    unsigned int lane_a[3];
    unsigned int lane_b[3];
} Axis5W18Aggregate;

static unsigned int axis5_w18_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w18_seed_snapshot(
    Axis5W18Snapshot* out,
    unsigned int shard,
    unsigned int frontier,
    unsigned int checkpoint,
    unsigned int lane_a,
    unsigned int lane_b
) {
    out->shard = shard % 3u;
    out->frontier = frontier;
    out->checkpoint = checkpoint;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w18_encode_snapshot(const Axis5W18Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x1du;
    wire[1] = s->frontier ^ 0x37u;
    wire[2] = s->checkpoint ^ 0x59u;
    wire[3] = s->lane_a ^ 0x73u;
    wire[4] = s->lane_b ^ 0x9du;
}

void axis5_w18_decode_snapshot(Axis5W18Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x1du;
    s->frontier = wire[1] ^ 0x37u;
    s->checkpoint = wire[2] ^ 0x59u;
    s->lane_a = wire[3] ^ 0x73u;
    s->lane_b = wire[4] ^ 0x9du;
}

void axis5_w18_clear_aggregate(Axis5W18Aggregate* a) {
    for (int shard = 0; shard < 3; ++shard) {
        a->frontier[shard] = 0u;
        a->checkpoint[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w18_absorb_snapshot(Axis5W18Aggregate* a, const Axis5W18Snapshot* s) {
    unsigned int shard = s->shard % 3u;
    if (s->frontier < a->frontier[shard]) {
        return;
    }
    if (s->frontier > a->frontier[shard]) {
        a->frontier[shard] = s->frontier;
        a->checkpoint[shard] = s->checkpoint;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
        return;
    }
    if (s->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = s->checkpoint;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
    } else if (s->checkpoint == a->checkpoint[shard]) {
        a->lane_a[shard] += s->lane_a;
        a->lane_b[shard] += s->lane_b;
    }
}

unsigned int axis5_w18_snapshot_signature(const Axis5W18Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 3; ++shard) {
        h = axis5_w18_snapshot_mix(h, a->frontier[shard]);
        h = axis5_w18_snapshot_mix(h, a->checkpoint[shard]);
        h = axis5_w18_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w18_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
