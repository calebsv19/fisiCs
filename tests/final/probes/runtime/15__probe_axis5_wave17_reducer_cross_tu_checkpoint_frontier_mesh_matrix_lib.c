typedef struct Axis5W17Snapshot {
    unsigned int shard;
    unsigned int checkpoint;
    unsigned int frontier;
    unsigned int lane_a;
    unsigned int lane_b;
} Axis5W17Snapshot;

typedef struct Axis5W17Aggregate {
    unsigned int checkpoint[3];
    unsigned int frontier[3];
    unsigned int lane_a[3];
    unsigned int lane_b[3];
} Axis5W17Aggregate;

static unsigned int axis5_w17_snapshot_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

void axis5_w17_seed_snapshot(
    Axis5W17Snapshot* out,
    unsigned int shard,
    unsigned int checkpoint,
    unsigned int frontier,
    unsigned int lane_a,
    unsigned int lane_b
) {
    out->shard = shard % 3u;
    out->checkpoint = checkpoint;
    out->frontier = frontier;
    out->lane_a = lane_a;
    out->lane_b = lane_b;
}

void axis5_w17_encode_snapshot(const Axis5W17Snapshot* s, unsigned int wire[5]) {
    wire[0] = s->shard ^ 0x17u;
    wire[1] = s->checkpoint ^ 0x39u;
    wire[2] = s->frontier ^ 0x5bu;
    wire[3] = s->lane_a ^ 0x7du;
    wire[4] = s->lane_b ^ 0x9fu;
}

void axis5_w17_decode_snapshot(Axis5W17Snapshot* s, const unsigned int wire[5]) {
    s->shard = wire[0] ^ 0x17u;
    s->checkpoint = wire[1] ^ 0x39u;
    s->frontier = wire[2] ^ 0x5bu;
    s->lane_a = wire[3] ^ 0x7du;
    s->lane_b = wire[4] ^ 0x9fu;
}

void axis5_w17_clear_aggregate(Axis5W17Aggregate* a) {
    for (int shard = 0; shard < 3; ++shard) {
        a->checkpoint[shard] = 0u;
        a->frontier[shard] = 0u;
        a->lane_a[shard] = 0u;
        a->lane_b[shard] = 0u;
    }
}

void axis5_w17_absorb_snapshot(Axis5W17Aggregate* a, const Axis5W17Snapshot* s) {
    unsigned int shard = s->shard % 3u;
    if (s->checkpoint < a->checkpoint[shard]) {
        return;
    }
    if (s->checkpoint > a->checkpoint[shard]) {
        a->checkpoint[shard] = s->checkpoint;
        a->frontier[shard] = s->frontier;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
        return;
    }
    if (s->frontier > a->frontier[shard]) {
        a->frontier[shard] = s->frontier;
        a->lane_a[shard] = s->lane_a;
        a->lane_b[shard] = s->lane_b;
    } else if (s->frontier == a->frontier[shard]) {
        a->lane_a[shard] += s->lane_a;
        a->lane_b[shard] += s->lane_b;
    }
}

unsigned int axis5_w17_snapshot_signature(const Axis5W17Aggregate* a) {
    unsigned int h = 2166136261u;
    for (int shard = 0; shard < 3; ++shard) {
        h = axis5_w17_snapshot_mix(h, a->checkpoint[shard]);
        h = axis5_w17_snapshot_mix(h, a->frontier[shard]);
        h = axis5_w17_snapshot_mix(h, a->lane_a[shard]);
        h = axis5_w17_snapshot_mix(h, a->lane_b[shard]);
    }
    return h;
}
